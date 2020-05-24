#pragma once
#include <string>
#include <windows.h>

class output_console
{
public:
	explicit output_console(std::string path)
	{
		STARTUPINFO si{ sizeof(si) };
		PROCESS_INFORMATION pi{};
		while (!path.empty() && path.back() != '\\') {
			path.pop_back();
		}
		const auto output_console_path = path + "output_console.exe";
		if (!CreateProcessA(output_console_path.c_str(), nullptr, nullptr, nullptr,
			TRUE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
		{
			const auto msg = "CreateProcess wailed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		output_console_process_ = pi.hProcess;
		pipe_ = CreateNamedPipeA(R"(\\.\pipe\output_console)", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE, 2, BUFSIZ, BUFSIZ, 0, nullptr);
		if (pipe_ == INVALID_HANDLE_VALUE) {
			const auto msg = "CreateNamedPipe failed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		client_output_received_msg_ = CreateSemaphoreA(nullptr, 1, 1, "client_output_received_msg");
		if (!client_output_received_msg_) {
			const auto msg = "CreateSemaphore failed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		if (client_output_received_msg_ && GetLastError() == ERROR_ALREADY_EXISTS) {
			throw std::exception("output_console wasn't close");
		}
		wait_for_output_ = CreateSemaphoreA(nullptr, 0, 1, "wait_for_output");
		if (!wait_for_output_) {
			const auto msg = "CreateSemaphore failed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		if (wait_for_output_ && GetLastError() == ERROR_ALREADY_EXISTS) {
			const auto msg = "CreateSemaphore failed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		ConnectNamedPipe(pipe_, nullptr);
	}
	BOOL write_to_console(const std::string &msg)
	{
		DWORD written;
		strcpy_s(write_buf_, BUFSIZ, msg.c_str());
		return WriteFile(pipe_, write_buf_, msg.size(), &written, nullptr);
	}
	void wait_output_received_msg() const
	{
		WaitForSingleObject(client_output_received_msg_, INFINITE);
	}
	void release_output_received_msg() const
	{
		ReleaseSemaphore(client_output_received_msg_, 1, nullptr);
	}
	void release_wait_for_output() const
	{
		ReleaseSemaphore(wait_for_output_, 1, nullptr);
	}
	~output_console()
	{
		FlushFileBuffers(pipe_);
		DisconnectNamedPipe(pipe_);
		CloseHandle(pipe_);
		CloseHandle(wait_for_output_);
		CloseHandle(client_output_received_msg_);
		TerminateProcess(output_console_process_, EXIT_SUCCESS);
		CloseHandle(output_console_process_);
	}
private:
	char write_buf_[BUFSIZ]{};
	HANDLE client_output_received_msg_;
	HANDLE wait_for_output_;
	HANDLE pipe_;
	HANDLE output_console_process_;
};
