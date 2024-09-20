#include <Windows.h>
#include <mmsystem.h>

#include "sound.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

#pragma comment(lib, "winmm.lib")

using namespace modules;

Sound::Sound() {}

Sound::~Sound() = default;

void Sound::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["play_sound"] = nullptr;
	visitor->builtin_functions["stop_sound"] = nullptr;
	visitor->builtin_functions["set_volume"] = nullptr;
}

void Sound::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["play_sound"] = [this, visitor]() {
		auto file_path = visitor->builtin_arguments[0]->get_s();
		std::wstring wfile_path = std::wstring(file_path.begin(), file_path.end());
		PlaySound(wfile_path.c_str(), NULL, SND_ASYNC | SND_FILENAME);

		};

	visitor->builtin_functions["stop_sound"] = [this, visitor]() {
		PlaySound(NULL, 0, 0);

		};

	visitor->builtin_functions["set_volume"] = [this, visitor]() {
		auto volume = visitor->builtin_arguments[0]->get_i();
		waveOutSetVolume(0, MAKELONG(volume, volume));

		};
}
