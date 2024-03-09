// Download Minesweeper at https://www.minesweeper.info/downloads/games/Winmine__XP.exe
// SHA256 BCFF89311D792F6428468E813AC6929A346A979F907071C302F418D128EAAF41
#include <windows.h>
#include <stdio.h>
#include <psapi.h>

void PrintSolution(HANDLE hProcess)
{
    printf("\nMinesweeper Found!!\n");
    UINT16 bytesToRead = 1;
    char* Width = new char[bytesToRead];
    char* Height = new char[bytesToRead];
    char* Mines = new char[bytesToRead];
    SIZE_T bytesRead;

    // Could be more elegant no ASLR is used so hard-coding is fine for 2001...
    // These offsets are observed in IDA as being the variables that store their respective values
    // We use this data to determine our matrix size and calculate how many mines to expect
    // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory
    ReadProcessMemory(hProcess, (LPCVOID)0x1005334, &Width, sizeof(Width), &bytesRead);
    ReadProcessMemory(hProcess, (LPCVOID)0x1005338, &Height, sizeof(Height), &bytesRead);
    //ReadProcessMemory(hProcess, (LPCVOID)0x1005330, &Mines, sizeof(Mines), &bytesRead); // Just for fun

    // Iterate through the matrix
    char* Mine = new char[bytesToRead];
    for (INT8 y = 0; y < (INT8)Height; y++)
    {
        for (INT8 x = 0; x < (INT8)Width; x++)
        {
            // Calculate the location of each cell in the matrix and get it's value
            // This again is something that can be observed in IDA
            INT32 CellAddress = (0x1005340) + (32 * (y + 1)) + (x + 1);
            ReadProcessMemory(hProcess, (LPCVOID)CellAddress, &Mine, sizeof(Mine), &bytesRead);

            // 'X' means mine, '.' means clear
            // The byte for a mine is 0x8f and the byte for no mine is 0xf
            if ((UINT8)Mine == 0x8f)
            {
                printf("X ");
            }
            else
            {
                printf(". ");

            }
        }
        printf("\n");
    }

    // Release the handle to the process
    CloseHandle(hProcess);
}

// Main takeaway - Read the documentation
int main(void)
{
    // Get the list of process identifiers
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        return 1;
    }

    // Calculate how many process identifiers were returned
    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process
    for (UINT16 i = 0; i < cProcesses; i++)
    {
        if (aProcesses[i] != 0)
        {
            // Get a handle to the process.
            // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);

            // Get the process name.
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
            if (NULL != hProcess)
            {
                HMODULE hMod;
                DWORD cbNeeded;

                // Get the module handle for this process
                // https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodulesex
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
                {
                    // Get  the name to see if it is Minesweeper
                    // https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getmodulebasenamea
                    GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));

                    // Convert to lowercase
                    for (UINT16 i = 0; szProcessName[i]; i++)
                    {
                        szProcessName[i] = tolower(szProcessName[i]);
                    }

                    if (!strncmp(szProcessName, "winmine", 7))
                    {
                        // We are good to start going
                        PrintSolution(hProcess);
                        return 0;
                    }
                }
            }
        }
    }
    printf("Minesweeper Not Found ;(");
    return 0;
}
