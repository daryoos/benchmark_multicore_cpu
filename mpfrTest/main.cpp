#include <iostream>

#include <mpreal.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <fstream>

#include <process.h>
#include <thread>
#include <string>
#include <future>

#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>

#include "Profiler.h"

#include <mutex>
#include <chrono>
#include <queue>
#include <functional>
#include <condition_variable>

#include <limits>
#undef max

typedef mpfr::mpreal mpreal;

#define rdtsc __asm __emit 0fh __asm __emit 031h
#define cpuid __asm __emit 0fh __asm __emit 0a2h

#define NUMBER_OF_TESTS 10
#define PERFORMANCE_LIMIT 5

//cpu specs
int numCores;
char cpuName[256];
unsigned int frequency;

unsigned int GetCpuFrequency();
void cpuSpecsPrint();
void cpuSpecs();

//profiler
Profiler paralelismTimes("paralelism");
void graphParalelism(int nr_threads, int& score);

//tests
float measureMultitaskingSpeed(int n);
int measureMaxThreads(int n, int prec, int& score);
float measureEncryption(int nrTest, bool en, int key, int n, int len, Operation& op);
float measureParalelism(std::vector<std::thread> threads, Operation& op);

//nthDigitPi
mpreal sqrt_custom(mpreal n, mpreal m);
mpreal power(int n);
mpreal pi(int prec);
void nthDigitPi(int n, int prec);
void threadDigitPi(int n, int prec, int nrThreads);

//encryption
std::mutex enMutex;

char msg[100000000];
char en[10000000];
char m[10000000];
char temp[10000000];

int prime(long int pr);
void encryption_key(long int keys[2], int x, int y, int t);
long int cd(long int a, int t);
void encrypt(long int key, int n, int start, int finish);
void decrypt(long int key, int n, int start, int finish);
void encryption(int x, int y, const int nr_threads, Operation& opEn, Operation& opDe, float& enTime, float& deTime);

//load balancing
std::mutex coutMutex;

struct Task {
	int id;
	int processingTime;

	Task(int taskId, int time) : id(taskId), processingTime(time) {}
};

class LoadBalancer {
private:
	std::vector<std::thread> workers;
	std::queue<Task> taskQueue;
	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;

	void workerFunction(int workerId) {
		while (true) {
			std::unique_lock<std::mutex> lock(queueMutex);

			condition.wait(lock, [this] { return !taskQueue.empty() || stop; });

			if (taskQueue.empty() && stop) {
				break;
			}

			Task task = taskQueue.front();
			taskQueue.pop();

			lock.unlock();

			std::this_thread::sleep_for(std::chrono::milliseconds(task.processingTime));

			{
				std::lock_guard<std::mutex> coutLock(coutMutex);
				std::cout << "		worker " << workerId << " completed task " << task.id
					<< " after " << task.processingTime << " milliseconds\n";
			}
		}
	}

public:
	LoadBalancer(int numWorkers) : stop(false) {
		for (int i = 0; i < numWorkers; ++i) {
			workers.emplace_back(&LoadBalancer::workerFunction, this, i);
		}
	}

	~LoadBalancer() {
		stop = true;
		condition.notify_all();

		for (auto& worker : workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}

	void enqueueTask(const Task& task) {
		std::lock_guard<std::mutex> lock(queueMutex);
		taskQueue.push(task);
		condition.notify_one();
	}
};

void loadBalancing(int numWorkers, int numTasks, int& score);

//main
int main()
{
	bool reset = true;
	while (reset) {
		int totalScore = 0;
		int paralelismScore = 0;
		int maxThreadsScore = 0;
		int loadBalancingScore = 0;

		system("cls");
		cpuSpecs();

		int testSelectKey = 0;

		std::cout << "--------------------------------------------------------------\n";
		std::cout << "Please select one:\n";
		std::cout << "	to run all tests press 1\n";
		std::cout << "	to run Paralelism Test press 2\n";
		std::cout << "	to run Maximum Thread Test press 3\n";
		std::cout << "	to run Load Balacing Test press 4\n";
		std::cout << "input: ";
		std::cin >> testSelectKey;
		std::cout << "--------------------------------------------------------------\n";
		while (!std::cin.good() || (testSelectKey != 1 && testSelectKey != 2 && testSelectKey != 3 && testSelectKey != 4)) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			system("cls");
			cpuSpecsPrint();
			std::cout << "--------------------------------------------------------------\n";
			std::cout << "Please select one:\n";
			std::cout << "	to run all tests press 1\n";
			std::cout << "	to run Paralelism Test press 2\n";
			std::cout << "	to run Maximum Thread Test press 3\n";
			std::cout << "	to run Load Balacing Test press 4\n";
			std::cout << "input: ";
			std::cin >> testSelectKey;
			std::cout << "--------------------------------------------------------------\n";
		}

		int precision;
		char nothing[256];
		int numWorkers = 0;
		int numTasks = 0;

		switch (testSelectKey) {
		case 1:
			system("cls");
			graphParalelism(5, paralelismScore);
			totalScore += paralelismScore;

			std::cout << "Press Enter to Continue";
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			/*
			std::cout << "\nWrite something and press enter to continue\n";
			std::cin >> nothing;
			std::cout << "\n";
			*/

			do {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				system("cls");
				std::cout << "Provide precision for calculating number pi (recommended 1000): ";
				std::cin >> precision;
				std::cout << "\n";
			} while (!std::cin.good());
			system("cls");
			measureMaxThreads(1, precision, maxThreadsScore);
			totalScore += maxThreadsScore;

			std::cout << "Press Enter to Continue";
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			do {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				system("cls");
				std::cout << "Provide inputs for number of workers and number of tasks:\n";
				std::cout << "	number of workers: ";
				std::cin >> numWorkers;
			} while (!std::cin.good());
			do {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				system("cls");
				std::cout << "Provide inputs for number of workers and number of tasks:\n";
				std::cout << "	number of workers: " << numWorkers << "\n";
				std::cout << "	number of tasks: ";
				std::cin >> numTasks;
				std::cout << "\n";
			} while (!std::cin.good());
			
			system("cls");
			loadBalancing(numWorkers, numTasks, loadBalancingScore);
			totalScore += loadBalancingScore;

			std::cout << "Write something and Press Enter to Continue";
			std::cin >> nothing;
			break;

		case 2:
			system("cls");
			graphParalelism(5, paralelismScore);
			totalScore += paralelismScore;

			std::cout << "Write something and Press Enter to Continue";
			std::cin >> nothing;
			break;
		
		case 3:
			system("cls");
			do {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				system("cls");
				std::cout << "Provide precision for calculating number pi (recommended 1000): ";
				std::cin >> precision;
				std::cout << "\n";
			} while (!std::cin.good());
			system("cls");
			measureMaxThreads(0, precision, maxThreadsScore);
			totalScore += maxThreadsScore;

			std::cout << "Press Enter to Continue";
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			break;

		case 4:
			do {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				system("cls");
				std::cout << "Provide inputs for number of workers and number of tasks:\n";
				std::cout << "	number of workers: ";
				std::cin >> numWorkers;
			} while (!std::cin.good());
			do {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				system("cls");
				std::cout << "Provide inputs for number of workers and number of tasks:\n";
				std::cout << "	number of workers: " << numWorkers << "\n";
				std::cout << "	number of tasks: ";
				std::cin >> numTasks;
				std::cout << "\n";
			} while (!std::cin.good());

			system("cls");
			loadBalancing(numWorkers, numTasks, loadBalancingScore);
			totalScore += loadBalancingScore;

			std::cout << "Write something and Press Enter to Continue";
			std::cin >> nothing;
			break;
			//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}

		int resetSelectKey;
		do {
			system("cls");
			std::cout << "--------------------------------------------------------------\n";
			std::cout << "Total score performed: " << totalScore << "\n";
			std::cout << "--------------------------------------------------------------\n";
			std::cout << "Please select one:\n";
			std::cout << "	to reset press 1\n";
			std::cout << "	to end press 2\n";
			std::cout << "input: ";
			std::cin >> resetSelectKey;
			std::cout << "--------------------------------------------------------------\n";
		} while (!std::cin.good() || (resetSelectKey != 1 && resetSelectKey != 2));
		if (resetSelectKey == 2) {
			reset = false;
		}
	}

	/*
	cpuSpecs();

	graphParalelism(3, paralelismScore);
	totalScore += paralelismScore;

	measureMaxThreads(120, 1000, maxThreadsScore);
	totalScore += maxThreadsScore;
	
	loadBalancing(4, 100, loadBalancingScore);
	totalScore += loadBalancingScore;
	*/

	//encryption(7, 19, 4);
	//nthDigitPi(120, 1000);

	return 0;
}

//cpu specs
unsigned int GetCpuFrequency() {
	unsigned int frequency = 0;

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr)) {
		hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
		if (SUCCEEDED(hr)) {
			IWbemLocator* pLoc = nullptr;
			hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
			if (SUCCEEDED(hr)) {
				IWbemServices* pSvc = nullptr;
				hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
				if (SUCCEEDED(hr)) {
					hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
					if (SUCCEEDED(hr)) {
						IEnumWbemClassObject* pEnumerator = nullptr;
						hr = pSvc->ExecQuery(
							bstr_t("WQL"),
							bstr_t("SELECT * FROM Win32_Processor"),
							WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
							nullptr,
							&pEnumerator);

						if (SUCCEEDED(hr)) {
							IWbemClassObject* pclsObj = nullptr;
							ULONG uReturn = 0;

							while (pEnumerator) {
								hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

								if (uReturn == 0)
									break;

								VARIANT vtProp;
								hr = pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
								frequency = vtProp.uintVal;

								pclsObj->Release();
							}
							pEnumerator->Release();
						}
					}
					pSvc->Release();
				}
				pLoc->Release();
			}
			CoUninitialize();
		}
	}

	return frequency;
}

void cpuSpecsPrint() {
	std::cout << "--------------------------------------------------------------\n";
	std::cout << "CPU Name: " << cpuName << "\n";
	std::cout << "CPU Frequency: " << frequency << "\n";
	std::cout << "Number of Cores: " << numCores << "\n";
	std::cout << "--------------------------------------------------------------\n";
	std::cout << "\n";
}

void cpuSpecs() {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	// Get CPU name
	DWORD size = sizeof(cpuName);
	RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", RRF_RT_REG_SZ, nullptr, cpuName, &size);

	// Get CPU frequency
	frequency = GetCpuFrequency();

	// Get number of cores and threads
	numCores = sysInfo.dwNumberOfProcessors;

	cpuSpecsPrint();
}

//profiler
void graphParalelism(int iterations, int& score) {
	std::cout << "--------------------------------------------------------------\n";
	std::cout << "Paralelism test:\n";

	int curr_threads = 0;
	for (int i = 0; i < iterations; i++) {
		Operation opEn = paralelismTimes.createOperation("encrypt_cycles", curr_threads);
		Operation opDe = paralelismTimes.createOperation("decrypt_cycles", curr_threads);

		if (curr_threads == 0) {
			std::cout << "	for 1 thread: " << "\n";
		}
		else {
			std::cout << "	for " << curr_threads << " threads: " << "\n";
		}

		float enTime;
		float deTime;
		encryption(7, 19, curr_threads, opEn, opDe, enTime, deTime);

		
		std::cout << "		encrypt time: " << enTime << "\n";
		std::cout << "		decrypt time: " << deTime << "\n";

		score += int(100000.0 / (enTime * 1000) + 10000.0 / (deTime * 1000));

		curr_threads += numCores;
	}
	paralelismTimes.reset();

	std::cout << "Score: " << score << "\n";
	std::cout << "--------------------------------------------------------------\n";
	std::cout << "\n";
}

//tests
float measureMultitaskingSpeed(int n) {
	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;
	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}

	for (int i = 0; i < n; i++) {

		cycles_high1 = 0;
		cycles_low1 = 0;
		//Measure the code sequence
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
		}

		//Section of code to be measured

		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high2, edx
			mov cycles_low2, eax
			popad
		}
		temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
		temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
		total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

		float sec_time = (float)total_cycles / (frequency * 1000000);
		if (sec_time > 0) {
			total_time += sec_time;
			//printf("%f\n", sec_time);
		}
		else {
			n++;
		}
	}
	float avg_time = total_time / NUMBER_OF_TESTS;
	return avg_time;
}

int measureMaxThreads(int n, int prec, int& score) {
	std::cout << "--------------------------------------------------------------\n";
	std::cout << "Maximum running threads test for calculating nTh digit of pi\n";

	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;
	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}

	int nrThreads = numCores;
	float minTime;
	while (true) {
		int nrTest = 1;
		for (int i = 0; i < nrTest; i++) {
			cycles_high1 = 0;
			cycles_low1 = 0;
			//Measure the code sequence
			__asm {
				pushad
				CPUID
				RDTSC
				mov cycles_high1, edx
				mov cycles_low1, eax
				popad
			}

			//Section of code to be measured
			threadDigitPi(n, prec, nrThreads);

			__asm {
				pushad
				CPUID
				RDTSC
				mov cycles_high2, edx
				mov cycles_low2, eax
				popad
			}
			temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
			temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
			total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

			float sec_time = (float)total_cycles / (frequency * 1000000);
			if (sec_time > 0) {
				std::cout << "	time for " << nrThreads << " threads: " << sec_time << "\n";
				if (nrThreads == numCores) {
					minTime = sec_time;
				}
				else {
					if (sec_time > PERFORMANCE_LIMIT * minTime) {
						score += nrThreads * 1000;
						score += int(100000.0 / sec_time);
						std::cout << "Score: " << score << "\n";
						std::cout << "--------------------------------------------------------------\n";
						std::cout << "\n";

						return nrThreads;
					}
				}
				nrThreads += numCores;
			}
			else {
				nrTest++;
			}
		}
	}
	return -1;
}

float measureEncryption(int nrTest, bool en, int key, int n, int len, Operation& op) {
	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;
	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}

	for (int i = 0; i < nrTest; i++) {

		cycles_high1 = 0;
		cycles_low1 = 0;
		//Measure the code sequence
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
		}

		//Section of code to be measured
		if (en == true)
			encrypt(key, n, 0, len);
		else
			decrypt(key, n, 0, len);

		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high2, edx
			mov cycles_low2, eax
			popad
		}
		temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
		temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
		total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

		float sec_time = (float)total_cycles / (frequency * 1000000);
		if (sec_time > 0) {
			total_time += sec_time;
			op.count(total_cycles);
			//printf("%f\n", sec_time);
		}
		else {
			nrTest++;
		}
	}
	float avg_time = total_time / nrTest;
	return avg_time;
}

float measureParalelism(std::vector<std::thread> threads, Operation &op) {
	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;
	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}


	cycles_high1 = 0;
	cycles_low1 = 0;
	//Measure the code sequence
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
	}

	//Section of code to be measured
	for (auto& thread : threads) {
		thread.join();
	}

	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high2, edx
		mov cycles_low2, eax
		popad
	}
	temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
	temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
	total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

	float sec_time = (float)total_cycles / (frequency * 1000000);
	op.count(long int(total_cycles));
	return sec_time;
}

//nthDigitPi
mpreal sqrt_custom(mpreal n, mpreal m) {
	mpreal m1 = pow(10, 16);
	mpreal m2 = static_cast<mpreal>((n * m1) / m) / m1;
	mpreal b = (static_cast<mpreal>(m1 * sqrt(m2)) * m) / m1;
	mpreal n_m = n * m;

	while (true) {
		mpreal a = b;
		b = (b + n_m / b) / 2;
		if (b == a) {
			break;
		}
	}

	return b;
}

mpreal power(int n) {
	if (n == 0) {
		return 1;
	}
	mpreal r = power(n / 2);
	if (n % 2 == 0) {
		return r * r;
	}
	return r * r * 10;
}

mpreal pi(int prec) {
	mpreal::set_default_prec(prec);  //precision
	mpreal m = power(100000);
	mpreal c = pow(640320, 3) / 24;
	mpreal n = 1;
	mpreal Ak = m;
	mpreal Asum = m;
	mpreal Bsum = 0;

	int max_iterations = 10000;

	while (Ak != 0) {
		Ak *= -(6 * n - 5) * (2 * n - 1) * (6 * n - 1);
		Ak /= n * n * n * c;
		Asum += Ak;
		Bsum += n * Ak;
		n = n + 1;

		max_iterations--;
		if (max_iterations == 0) {
			break;
		}
	}

	return (426880.0 * sqrt_custom(10005 * m, m)) / (13591409 * Asum + 545140134 * Bsum);
}

void nthDigitPi(int n, int prec) {
	mpreal result = pi(prec);
	std::string stringPi = result.toString();

	/*
	if (n < stringPi.size()) {
		std::cout << "Digit at position " << n << " of pi is " << stringPi.substr(n + 1, 1) << std::endl;
	}
	else {
		std::cout << "Error" << std::endl;
	}
	*/
}

void threadDigitPi(int n, int prec, int nrThreads) {
	std::vector<std::thread> threads;

	for (int i = 0; i < nrThreads; i++) {
		nthDigitPi(120, 1000);
		threads.emplace_back(nthDigitPi, n, prec);
	}

	for (auto& thread : threads) {
		thread.join();
	}
}

//encryption
int prime(long int pr)
{
	int j = sqrt(pr);
	for (int i = 2; i <= j; i++)
	{
		if (pr % i == 0)
			return 0;
	}
	return 1;
}

void encryption_key(long int keys[2], int x, int y, int t)
{
	int flag;
	for (int i = 2; i < t; i++)
	{
		if (t % i == 0)
			continue;
		flag = prime(i);
		if (flag == 1 && i != x && i != y)
		{
			keys[0] = i;
			flag = cd(keys[0], t);
			if (flag > 0)
			{
				keys[1] = flag;
				return;
			}
		}
	}
}

long int cd(long int a, int t)
{
	long int k = 1;
	while (1)
	{
		k = k + t;
		if (k % a == 0)
			return(k / a);
	}
}

void encrypt(long int key, int n, int start, int finish)
{
	long int pt, ct, k;
	int i = start;

	while (i != finish)
	{
		pt = msg[i];
		pt = pt - 96;

		k = 1;
		for (int j = 0; j < key; j++)
		{
			k = k * pt;
			k = k % n;
		}
		temp[i] = k;
		ct = k + 96;
		en[i] = ct;
		i++;
	}
}

void decrypt(long int key, int n, int start, int finish)
{
	long int pt, ct, k;
	int i = start;
	while (i != finish)
	{
		ct = temp[i];

		k = 1;
		for (int j = 0; j < key; j++)
		{
			k = k * ct;
			k = k % n;
		}
		pt = k + 96;
		m[i] = pt;
		i++;
	}
}

void tencrypt(int tid, long int key, int n, int start, int finish) {
	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;

	//measureStart
	total_time = 0.0;

	cycles_high1 = 0;
	cycles_low1 = 0;
	cpuid_time = 0;
	cycles_high2 = 0;
	cycles_low2 = 0;
	temp_cycles1 = 0;
	temp_cycles2 = 0;
	total_cycles = 0;
	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}


	cycles_high1 = 0;
	cycles_low1 = 0;

	//Measure the code sequence
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
	}

	encrypt(key, n, start, finish);

	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high2, edx
		mov cycles_low2, eax
		popad
	}
	temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
	temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
	total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

	std::lock_guard<std::mutex> lock(enMutex);
	float tTime = (float)total_cycles / (frequency * 1000000);
	std::cout << "			thread " << tid << " finished in " << tTime << "\n";
}

void tdecrypt(int tid, long int key, int n, int start, int finish){
	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;

	//measureStart
	total_time = 0.0;

	cycles_high1 = 0;
	cycles_low1 = 0;
	cpuid_time = 0;
	cycles_high2 = 0;
	cycles_low2 = 0;
	temp_cycles1 = 0;
	temp_cycles2 = 0;
	total_cycles = 0;
	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}


	cycles_high1 = 0;
	cycles_low1 = 0;

	std::vector<std::thread> threads_encrypt;
	std::vector<std::thread> threads_decrypt;

	//Measure the code sequence
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
	}

	decrypt(key, n, start, finish);

	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high2, edx
		mov cycles_low2, eax
		popad
	}
	temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
	temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
	total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

	std::lock_guard<std::mutex> lock(enMutex);
	float tTime = (float)total_cycles / (frequency * 1000000);
	std::cout << "			thread " << tid << " finished in " << tTime << "\n";
}

void encryption(int x, int y, const int nr_threads, Operation& opEn, Operation& opDe, float& enTime, float& deTime) {
	int flag;

	flag = prime(x);
	if (flag == 0) {
		exit(0);
	}

	flag = prime(y);
	if (flag == 0 || x == y) {
		exit(0);
	}

	std::ifstream f;
	std::ofstream fe;
	std::ofstream fd;

	f.open("message.txt", std::ifstream::in);
	fe.open("emessage.txt", std::ofstream::out | std::ofstream::trunc);
	fd.open("dmessage.txt", std::ofstream::out | std::ofstream::trunc);

	int i = 0;
	while (f) {
		msg[i++] = f.get();
	}
	msg[i] = '\0';
	f.close();

	int n = x * y;
	int t = (x - 1) * (y - 1);

	long int keys[2] = { 0, 0 };
	encryption_key(keys, x, y, t);

	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;

	if (nr_threads == 0) {
		enTime = measureEncryption(1, true, keys[0], n, strlen(msg), opEn);
		deTime = measureEncryption(1, false, keys[1], n, strlen(msg), opDe);

		fe << en;
		fd << m;
	}
	else {
		//measureStart
		total_time = 0.0;

		cycles_high1 = 0;
		cycles_low1 = 0;
		cpuid_time = 0;
		cycles_high2 = 0;
		cycles_low2 = 0;
		temp_cycles1 = 0;
		temp_cycles2 = 0;
		total_cycles = 0;
		//compute the CPUID overhead
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
			pushad
			CPUID
			RDTSC
			popad
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
			pushad
			CPUID
			RDTSC
			popad
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
			pushad
			CPUID
			RDTSC
			sub eax, cycles_low1
			mov cpuid_time, eax
			popad
		}


		cycles_high1 = 0;
		cycles_low1 = 0;

		std::vector<std::thread> threads_encrypt;
		std::vector<std::thread> threads_decrypt;

		int len = strlen(msg) / nr_threads;
		for (int i = 0; i < nr_threads; i++) {
			if (i == nr_threads - 1) {
				threads_encrypt.emplace_back(tencrypt, i + 1, keys[0], n, i * len, strlen(msg));
			}
			else {
				threads_encrypt.emplace_back(tencrypt, i + 1, keys[0], n, i * len, (i + 1) * len);
			}
		}

		//Measure the code sequence
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
		}


		for (auto& thread : threads_encrypt) {
			thread.join();
		}

		//measureStop
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high2, edx
			mov cycles_low2, eax
			popad
		}
		temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
		temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
		total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

		enTime = (float)total_cycles / (frequency * 1000000);
		opEn.count(long int(total_cycles));

		fe << en;

		for (int i = 0; i < nr_threads; i++) {
			if (i == nr_threads - 1) {
				threads_decrypt.emplace_back(tdecrypt, i + nr_threads, keys[1], n, i * len, strlen(msg));
			}
			else {
				threads_decrypt.emplace_back(tdecrypt, i + nr_threads, keys[1], n, i * len, (i + 1) * len);
			}
		}

		//measureStart
		total_time = 0.0;

		cycles_high1 = 0;
		cycles_low1 = 0;
		cpuid_time = 0;
		cycles_high2 = 0;
		cycles_low2 = 0;
		temp_cycles1 = 0;
		temp_cycles2 = 0;
		total_cycles = 0;
		//compute the CPUID overhead
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
			pushad
			CPUID
			RDTSC
			popad
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
			pushad
			CPUID
			RDTSC
			popad
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
			pushad
			CPUID
			RDTSC
			sub eax, cycles_low1
			mov cpuid_time, eax
			popad
		}


		cycles_high1 = 0;
		cycles_low1 = 0;
		//Measure the code sequence
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high1, edx
			mov cycles_low1, eax
			popad
		}

		for (auto& thread : threads_decrypt) {
			thread.join();
		}
		
		//measureStop
		__asm {
			pushad
			CPUID
			RDTSC
			mov cycles_high2, edx
			mov cycles_low2, eax
			popad
		}
		temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
		temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
		total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

		deTime = (float)total_cycles / (frequency * 1000000);
		opDe.count(long int(total_cycles));
		fd << m;
	}
	fe.close();
	fd.close();
}

//load balacing
void loadBalancing(int numWorkers, int numTasks, int& score) {
	std::cout << "--------------------------------------------------------------\n";
	std::cout << "Load balancing test for " << numWorkers << " and " << numTasks << "\n";
	std::cout << "	Workers timers:\n";

	float time = 0;

	LoadBalancer loadBalancer(numWorkers);

	//measureStart
	float total_time = 0.0;

	unsigned cycles_high1 = 0, cycles_low1 = 0, cpuid_time = 0;
	unsigned cycles_high2 = 0, cycles_low2 = 0;
	unsigned __int64 temp_cycles1 = 0, temp_cycles2 = 0;
	__int64 total_cycles = 0;

	//compute the CPUID overhead
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		popad
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
		pushad
		CPUID
		RDTSC
		sub eax, cycles_low1
		mov cpuid_time, eax
		popad
	}


	cycles_high1 = 0;
	cycles_low1 = 0;
	//Measure the code sequence
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high1, edx
		mov cycles_low1, eax
		popad
	}

	for (int i = 0; i < numTasks; ++i) {
		int processingTime = (i % numWorkers + 1) * 100; // Varying processing times
		Task task(i, processingTime);
		loadBalancer.enqueueTask(task);
	}
	loadBalancer.~LoadBalancer();

	//measureStop
	__asm {
		pushad
		CPUID
		RDTSC
		mov cycles_high2, edx
		mov cycles_low2, eax
		popad
	}
	temp_cycles1 = ((unsigned __int64)cycles_high1 << 32) | cycles_low1;
	temp_cycles2 = ((unsigned __int64)cycles_high2 << 32) | cycles_low2;
	total_cycles = temp_cycles2 - temp_cycles1 - cpuid_time;

	time = (float)total_cycles / (frequency * 1000000);

	score += int(100000.0 / time);
	score *= float(numTasks) / float(numWorkers) / 10.0;

	std::cout << "	Time to complete all tasks: " << time << "\n";
	std::cout << "Score: " << score << "\n";
	std::cout << "--------------------------------------------------------------\n";
}
