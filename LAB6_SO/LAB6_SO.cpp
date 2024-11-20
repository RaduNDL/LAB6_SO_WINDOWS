#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <sstream>
#include <cstdlib>
using namespace std;
bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}
void findPrimes(int start, int end, HANDLE writePipe) {
    DWORD bytesWritten;
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) {
            WriteFile(writePipe, &i, sizeof(i), &bytesWritten, NULL);
        }
    }
    int endSignal = -1; 
    WriteFile(writePipe, &endSignal, sizeof(endSignal), &bytesWritten, NULL);
}
int main(int argc, char* argv[]) {
    if (argc == 3) {
        int start = atoi(argv[1]);
        int end = atoi(argv[2]);
        HANDLE writePipe = GetStdHandle(STD_OUTPUT_HANDLE);
        findPrimes(start, end, writePipe);
        return 0;
    }
    const int TOTAL_NUMBERS = 10000;
    const int PROCESS_COUNT = 10;
    const int RANGE_SIZE = TOTAL_NUMBERS / PROCESS_COUNT;

    HANDLE pipes[PROCESS_COUNT][2]; 
    PROCESS_INFORMATION pi[PROCESS_COUNT]; 
    STARTUPINFOA si[PROCESS_COUNT]; 
    for (int i = 0; i < PROCESS_COUNT; ++i) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE; 
        if (!CreatePipe(&pipes[i][0], &pipes[i][1], &sa, 0)) {
            cerr << "Eroare la crearea pipe-ului!" << endl;
            return 1;
        }
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        si[i].hStdOutput = pipes[i][1]; 
        si[i].dwFlags |= STARTF_USESTDHANDLES;
        int start = i * RANGE_SIZE + 1;
        int end = start + RANGE_SIZE - 1;
        stringstream command;
        command << "\"" << argv[0] << "\" " << start << " " << end;
        ZeroMemory(&pi[i], sizeof(pi[i]));
        if (!CreateProcessA(NULL, const_cast<LPSTR>(command.str().c_str()),
            NULL, NULL, TRUE, 0, NULL, NULL, &si[i], &pi[i])) {
            cerr << "Eroare la crearea procesului!" << endl;
            return 1;
        }
        CloseHandle(pipes[i][1]);
    }
    for (int i = 0; i < PROCESS_COUNT; ++i) {
        DWORD bytesRead;
        int prime;
        cout << "Prime numbers from process " << i + 1 << ": ";
        while (true) {
            if (ReadFile(pipes[i][0], &prime, sizeof(prime), &bytesRead, NULL) && bytesRead > 0) {
                if (prime == -1) break; 
                cout << prime << " ";
            }
            else {
                break;
            }
        }
        cout << endl;
        CloseHandle(pipes[i][0]);
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }
    return 0;
}