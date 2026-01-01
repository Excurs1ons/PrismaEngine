#pragma once

#include "../ISubSystem.h"
#include "../audio/AudioBackend.h"
#include "WorkerThread.h"

#include <memory>
#include <string>

namespace PrismaEngine {
	namespace Audio {

		//CRTP模式
		class AudioSystem : public ISubSystem{
		public:
			AudioSystem();
			~AudioSystem();
            bool Initialize(AudioBackendType audioBackendType, AudioFormat audioFormat);
			bool Initialize() override;
			void Shutdown() override;
			bool PlayAudioFile(const char* filename);

		private:
			std::unique_ptr<AudioBackend> audioBackend;
            WorkerThread workerThread;
		};
	}
}
