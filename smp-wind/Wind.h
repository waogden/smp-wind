#pragma once

#include "PluginAPI.h"

namespace wind
{
	class Config;

	class Wind final : public hdt::IPreStepListener
	{
	private:
		class ThreadPool
		{
		public:
			static constexpr int MAX_THREADS = 128;

		public:
			ThreadPool(int count);
			ThreadPool(const ThreadPool&) = delete;

			~ThreadPool();

			ThreadPool& operator=(const ThreadPool&) = delete;

			int size() const { return (int)m_threads.size(); }

			void release(Wind* target);
			void wait() { m_barrier.arrive_and_wait(); }

		private:
			void worker();

		private:
			std::vector<std::thread> m_threads;
			std::counting_semaphore<MAX_THREADS> m_signal{ 0 };
			std::barrier<> m_barrier;
			Wind* m_target{ nullptr };
		};

	public:
		Wind() = default;
		~Wind() override = default;

		RE::BSEventNotifyControl ProcessEvent(const hdt::PreStepEvent* e, RE::BSTEventSource<hdt::PreStepEvent>*) override;

		void init(const Config& config);
		void updateThreadCount();

	private:
		void onEvent(const hdt::PreStepEvent& e);

		btVector3 eval(const btVector3& at);
		void process(btCollisionObject* object);
		void processThreadsafe();

	private:
		const RE::Sky* m_sky{ nullptr };
		const Config* m_config{ nullptr };

		float m_currentTime{ 0.0f };
		btVector3 m_currentDir;
		btVector3 m_orthoDir;

		std::unique_ptr<ThreadPool> m_threadPool;
		const btAlignedObjectArray<btCollisionObject*>* m_objArr{ nullptr };
		std::atomic<int> m_nextElement{ -1 };
	};
}
