#include "pch.h"
#include "Config.h"
#include "Timer.h"
#include "Wind.h"

namespace
{
	// Layout of FSMP's hdt::SkinnedMeshBone (src/hdtSkinnedMesh/hdtSkinnedMeshBone.h):
	//   +0x00 vfptr, +0x08 RE::BSIntrusiveRefCounted, +0x10 RE::BSFixedString m_name, ... btRigidBody m_rig
	// FSMP sets m_rig.setUserPointer(this), so the rigid body's user pointer is the bone.
	// m_name is the skeleton node name (possibly prefixed by FSMP's auto-rename).
	// NOTE: This is NOT part of FSMP's public API and may break without warning in a future FSMP version.
	constexpr std::uintptr_t BONE_NAME_OFFSET = 0x10;

	constexpr std::string_view AUTORENAME_PREFIX = "hdtSSEPhysics_AutoRename_"sv;

	const char* getBoneName(const btRigidBody* body)
	{
		auto bone = reinterpret_cast<std::uintptr_t>(body->getUserPointer());
		if (!bone) {
			return nullptr;
		}

		auto name = *reinterpret_cast<const char* const*>(bone + BONE_NAME_OFFSET);
		if (!name) {
			return nullptr;
		}

		// "hdtSSEPhysics_AutoRename_Armor_XXXXXXXX <bone>" / "hdtSSEPhysics_AutoRename_Head_XXXXXXXX <bone>"
		std::string_view sv(name);
		if (sv.starts_with(AUTORENAME_PREFIX)) {
			auto space = sv.find(' ', AUTORENAME_PREFIX.size());
			if (space != std::string_view::npos) {
				return name + space + 1;
			}
		}
		return name;
	}
}

static float randf(float min = 0.0f, float max = 1.0f)
{
	thread_local std::minstd_rand rng;

	assert(max > min);
	std::uniform_real_distribution<float> d(min, max);
	return d(rng);
}

RE::BSEventNotifyControl wind::Wind::ProcessEvent(const hdt::PreStepEvent* e, RE::BSTEventSource<hdt::PreStepEvent>*)
{
	if (e) {
		onEvent(*e);
	}
	return RE::BSEventNotifyControl::kContinue;
}

void wind::Wind::onEvent(const hdt::PreStepEvent& e)
{
	constexpr int NFRAMES = 120;

	static std::vector<int> frameTimes(NFRAMES);
	static std::vector<int> frameObjs(NFRAMES);
	static int frame = 0;

	Timer<int, std::micro> timer;


	assert(m_config);

	m_currentTime += e.timeStep;
	m_sky = RE::Sky::GetSingleton();
	if (m_sky && m_sky->mode == RE::Sky::Mode::kFull && m_sky->windSpeed != 0.0f) {

		if (e.objects.size() > 0) {

			const float dir = m_sky->windAngle;
			m_currentDir = btVector3(std::cos(dir), std::sin(dir), 0.0f);
			m_orthoDir = btVector3(std::cos(dir - 1.5708f), std::sin(dir - 1.5708f), 0.0f);

			if (m_threadPool && e.objects.size() > m_config->geti(Config::MULTITHREAD_THRESHOLD)) {

				m_objArr = &e.objects;
				m_nextElement.store(0);

				m_threadPool->release(this);

				processThreadsafe();

				m_threadPool->wait();
			}
			else {
				for (int i = 0; i < e.objects.size(); i++) {
					process(e.objects[i]);
				}
			}
		}
	}
	else {
		//We'll lose time resolution if the game runs for several hours (possible!).
		//To prevent this, reset the clock when the player is indoors.
		m_currentTime = 0.0f;
	}

	if (m_config->getb(Config::LOG_PERFORMANCE)) {
		frameObjs[frame] = e.objects.size();
		frameTimes[frame++] = timer.elapsed();

		if (frame == NFRAMES) {

			frame = 0;

			int meant = 0;
			int maxt = 0;
			int mint = std::numeric_limits<int>::max();
			int maxo = 0;
			int mino = std::numeric_limits<int>::max();
			for (int i = 0; i < NFRAMES; i++) {
				meant += frameTimes[i];
				maxt = std::max(frameTimes[i], maxt);
				mint = std::min(frameTimes[i], mint);
				maxo = std::max(frameObjs[i], maxo);
				mino = std::min(frameObjs[i], mino);
			}
			meant /= NFRAMES;

			float var = 0.0f;
			for (auto f : frameTimes) {
				var += (f - meant) * (f - meant);
			}
			var /= NFRAMES;

			_MESSAGE("Mean time (%d updates): %3d ± %-3d (%3d - %-3d) microseconds (%3d - %-3d collision objects)",
				NFRAMES, meant, (int)std::sqrt(var), mint, maxt, mino, maxo);
		}
	}
}

void wind::Wind::init(const Config& config)
{
	m_config = &config;
	updateThreadCount();
}

void wind::Wind::updateThreadCount()
{
	int threads = std::min(m_config->geti(Config::THREADS), ThreadPool::MAX_THREADS);

	if (m_threadPool && threads != m_threadPool->size() + 1) {
		m_threadPool.reset();
	}

	if (threads > 1) {
		m_threadPool = std::make_unique<ThreadPool>(threads - 1);
	}
}

btVector3 wind::Wind::eval(const btVector3& at)
{
	assert(m_sky && m_config);

	//Let wind speed increase linearly with height above sea level (z ~= -14000)
	float speed = (1.0f + m_config->getf(Config::HEIGHTFACTOR) * (1.0e-4f * at[2] + 1.4f)) * m_sky->windSpeed;

	if (speed > 0.0f) {

		//phase velocity parallel to the wind should be consistent with speed
		//largest speed in vanilla weathers is 1, which should correspond to something like 10-20 m/s (700-1400 units/s)
		float U = at.dot(m_currentDir) / (1000.f * speed);
		//phase velocity orthogonal to the wind is scaled arbitrarily
		float V = 5.0e-3f * at.dot(m_orthoDir);

		float phase01 = m_config->getf(Config::OSC01FREQ) * (m_currentTime - U - V);
		float phase02 = m_config->getf(Config::OSC02FREQ) * (m_currentTime - U - V);

		//Two different frequencies makes fluctuations less predictable
		btVector3 base = (1.0f + 0.5f * m_config->getf(Config::OSC01FORCE) * (std::cos(phase01) + std::cos(0.2692f * phase01))) * m_currentDir;

		float angle = m_sky->windAngle + 0.5f * m_config->getf(Config::OSC02SPAN) * (std::cos(phase02) + std::cos(0.3101f * phase02));
		btVector3 spread = m_config->getf(Config::OSC02FORCE) * btVector3(std::cos(angle), std::sin(angle), 0.0f);

		btVector3 noise = m_config->getf(Config::NOISE) * btVector3(randf(-1.0f, 1.0f), randf(-1.0f, 1.0f), randf(-1.0f, 1.0f));

		return m_config->getf(Config::FORCE) * speed * (base + spread + noise);
	}
	else {
		return btVector3(0.0f, 0.0f, 0.0f);
	}
}

void wind::Wind::process(btCollisionObject* object)
{
	assert(m_config);

	btRigidBody* body = btRigidBody::upcast(object);
	if (body && !body->isStaticOrKinematicObject()) {

		float factor = 1.0;

		if (m_config->hasBoneFactors()) {

			if (auto name = getBoneName(body)) {
				factor = m_config->getBoneFactor(name);
			}
		}

		if (factor > 0.0) {
			//scale by 100 * m, since that's the oom we adapted the wind for
			float rescale = m_config->getb(Config::MASS_INDEPENDENT) ? 100.0f * body->getMass() : 1.0f;
			body->applyCentralForce(factor * rescale * eval(body->getWorldTransform().getOrigin()));
		}
	}
}

void wind::Wind::processThreadsafe()
{
	assert(m_objArr);

	for (int i = m_nextElement.fetch_add(1); i < m_objArr->size(); i = m_nextElement.fetch_add(1)) {
		process((*m_objArr)[i]);
	}
}

wind::Wind::ThreadPool::ThreadPool(int count) :
	m_barrier(count + 1)
{
	assert(count > 0 && count <= MAX_THREADS);

	m_threads.resize(count);

	for (auto&& thread : m_threads) {
		thread = std::thread(&ThreadPool::worker, this);
	}
}

wind::Wind::ThreadPool::~ThreadPool()
{
	release(nullptr);

	for (auto&& thread : m_threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

void wind::Wind::ThreadPool::release(Wind* target)
{
	m_target = target;
	m_signal.release(m_threads.size());
}

void wind::Wind::ThreadPool::worker()
{
	while (true) {
		m_signal.acquire();

		if (m_target) {
			m_target->processThreadsafe();
			m_barrier.arrive_and_wait();
		}
		else {
			break;
		}
	}
}
