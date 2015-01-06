#include "stdafx.h"

int main(int argc, char **argv)
{
    using namespace std;
    if (argc < 4 ||
        lstrcmpiA(argv[1], "/?") == 0 || lstrcmpiA(argv[1], "--help") == 0)
    {
        printf("Usage: redirector input_file output_file error_file program [parameters]");
        return 0;
    }

    bool same_file = false;
    MFile input_file, output_file, error_file;
    if (lstrcmpiA(argv[2], argv[3]) == 0) {
        same_file = true;
    }

    if (!input_file.OpenFileForInput(argv[1])) {
        fprintf(stderr, "ERROR: Cannot open file '%s'.\n", argv[1]);
        return 1;
    }

    if (same_file) {
        if (!output_file.OpenFileForOutput(argv[2])) {
            fprintf(stderr, "ERROR: Cannot open file '%s'.\n", argv[2]);
            return 1;
        }
    } else {
        if (!output_file.OpenFileForOutput(argv[2])) {
            fprintf(stderr, "ERROR: Cannot open file '%s'.\n", argv[2]);
            return 1;
        }
        if (!error_file.OpenFileForOutput(argv[3])) {
            fprintf(stderr, "ERROR: Cannot open file '%s'.\n", argv[3]);
            return 2;
        }
    }

    tstring cmdline;
    for (int i = 4; i < argc; ++i) {
        cmdline += TEXT("\"");
        cmdline += argv[i];
        cmdline += TEXT("\" ");
    }
    fprintf(stderr, "CommandLine: %s\n", cmdline.data());

    MProcessMaker pmaker;
    MFile input, output, error;

    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    BOOL bOK;
    if (same_file) {
        bOK = pmaker.PrepareForRedirect(&input, &output, &output) &&
            pmaker.CreateProcess(NULL, cmdline.data());
    } else {
        bOK = pmaker.PrepareForRedirect(&input, &output, &error) &&
            pmaker.CreateProcess(NULL, cmdline.data());
    }

    if (!bOK) {
        fprintf(stderr, "ERROR: Cannot create process\n");
        return 3;
    }

    DWORD cbAvail, cbRead;
    const DWORD cbBuf = 2048;
    static BYTE szBuf[2048];

    while (input_file.ReadFile(szBuf, cbBuf, &cbRead) && cbRead > 0) {
        input.WriteFile(szBuf, cbRead, &cbRead);
    }
    input.CloseHandle();
    input_file.CloseHandle();

    for (;;) {
        cbAvail = 0;
        if (output.PeekNamedPipe(NULL, 0, NULL, &cbAvail) && cbAvail > 0) {
            if (output.ReadFile(szBuf, cbBuf, &cbRead)) {
                if (!output_file.WriteFile(szBuf, cbRead, &cbRead)) {
                    fprintf(stderr, "ERROR: Cannot write to file '%s'\n", argv[2]);
                    return 4;
                }
            } else {
                DWORD dwError = ::GetLastError();
                if (dwError != ERROR_MORE_DATA) {
                    fprintf(stderr, "ERROR: Cannot read output\n");
                    return 5;
                }
            }
        } else if (!same_file) {
            if (error.PeekNamedPipe(NULL, 0, NULL, &cbAvail) && cbAvail > 0) {
                if (error.ReadFile(szBuf, cbBuf, &cbRead)) {
                    error_file.WriteFile(szBuf, cbRead, &cbRead);
                    if (cbRead == 0) {
                        fprintf(stderr,
                            "ERROR: Cannot write to file '%s'\n",
                            argv[3]);
                        return 6;
                    }
                }
            } else if (!pmaker.IsRunning()) {
                break;
            }
        } else {
            if (!pmaker.IsRunning()) {
                break;
            }
        }
    }

    output_file.CloseHandle();
    if (!same_file)
        error_file.CloseHandle();

    DWORD dwExitCode = pmaker.GetExitCode();
    return dwExitCode;
}
