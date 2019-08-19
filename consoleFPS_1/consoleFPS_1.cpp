#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <utility>
using namespace std;

#include <stdio.h>
#include <Windows.h>

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;

int nMapHeight = 16;
int nMapWidth =  16;

float fFOV = 3.14159/4.0;
float fDepth = 16.0f;

int main()
{
	// Create Screen Buffer
    wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight]; // 2D array
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;
    
    // Create map
    wstring map;

	map += L"#########.......";
	map += L"#...............";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";
    // Need 2 time points
    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    // Game Loop
    while (1) {

        // get current system time
        tp2 = chrono::system_clock::now();
        // calculate duration between current and prev time
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // Controls
        // Handle counterclockwise rotation

        // rotate left
        if (GetAsyncKeyState((unsigned short) 'A') & 0x8000){
            fPlayerA -=(0.8f) *fElapsedTime;
        }
        // rotate right
         if (GetAsyncKeyState((unsigned short) 'D') & 0x8000){
            fPlayerA +=(0.8f) *fElapsedTime;
        }
        // go forward
         if (GetAsyncKeyState((unsigned short) 'W') & 0x8000){
            fPlayerX += sinf(fPlayerA) * 5.0f *fElapsedTime;
            fPlayerY += cosf(fPlayerA) * 5.0f *fElapsedTime;

            // Collision detection
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
                fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        // go forward
         if (GetAsyncKeyState((unsigned short) 'S') & 0x8000){
            fPlayerX -= sinf(fPlayerA) * 5.0f *fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * 5.0f *fElapsedTime;
            
            // Collision detection
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
                fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        

        // Take each column of the console & relating it to ray cast to field of
        // view. Cast 120 rays into scene.
        for(int x = 0; x < nScreenWidth; x++){
            // for each column calculate the projected ray angle into world space 
            float fRayAngle =  (fPlayerA - fFOV / 2.0f) + ((float)x/(float)nScreenWidth) * fFOV;

            float fDistanceToWall = 0.0f;
            bool bHitWall = false;
            bool bBoundary = false;
            
            float fEyeX = sinf(fRayAngle); //Unit vector for ray in player space
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth) {
                fDistanceToWall += 0.1f;
                // only interested in integer part
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else {
                    // Ray is inbounds so test to see if the ray cell is a wall #  block
                    if (map[nTestY * nMapWidth + nTestX] == '#') {
                        bHitWall = true;

                        // Corner detection. Cast rays from each wall corner
                        // and find which 2 are the closest to player
                        vector<pair<float, float>> positionVector; //distance, dot
                        for (int tx = 0; tx <= 2; tx++){
                            for (int ty = 0; ty <= 2; ty++){
                                // Get vectors from player from perfect corner
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;

                                // distance from corner = sqrt((x2 -x1)^2 + (y2-y1)^2))
                                // where x2 is corner and x1 is player location
                                
                                // calculate distance from corners
                                float d = sqrt(vx*vx + vy*vy);
                                // dot product = (ax * bx) + (ay *by).
                                // Representation of angle between the ray being cast at the moment (unit vectors fEyeX and fEyeY) 
                                // and unit vector of perfect corner (vx/d and vy/d).
                                float dot = (fEyeX * vx /d) + (fEyeY * vy / d);
                                // Add to vector, and then sort
                                positionVector.push_back(make_pair(d, dot));
                            }

                            // Sort pairs from closest to corner to farthest
                            // using a lambda function
                            sort(positionVector.begin(), positionVector.end(), [](const pair<float,float> & left, const pair<float,float> &right){
                                    return left.first < right.first;});
                        
                            float fBound = 0.01;
                            // inverse cosine of dot product, you get angle between 2 rays. Check only the 
                            // three closest corners, we'll never see all 4
                            if (acos(positionVector.at(0).second) < fBound) bBoundary = true;
                            if (acos(positionVector.at(1).second) < fBound) bBoundary = true;
							if (acos(positionVector.at(2).second) < fBound) bBoundary = true;

                            }
                        }
                }
            }

        // Calculate distance to ceiling and floor
        // As distance to wall gets larger, subtraction gets smaller, therefore we
        // have a higher ceiling
        int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
        int nFloor = nScreenHeight - nCeiling; 

        short nShade = ' ';

        if (fDistanceToWall <= fDepth / 4.0f)       nShade = 0x2588; // Very close
        else if (fDistanceToWall < fDepth / 3.0f)   nShade = 0x2593;
        else if (fDistanceToWall < fDepth / 2.0f)   nShade = 0x2592;
        else if (fDistanceToWall < fDepth)          nShade = 0x2591;
        else                                        nShade = ' ';    // Too far away
    
        if (bBoundary)                              nShade = ' '; //Black out wall corners

        for(int y = 0; y < nScreenHeight; y++) {
            //Sky is blank space
            if (y <= nCeiling){
                screen[y*nScreenWidth + x] = ' ';
            }
            // Must be wall
            else if(y > nCeiling && y <= nFloor){
                screen[y*nScreenWidth + x] = nShade;
            }
            // If neither ceiling or wall, must be floor
            else{
                 // Shade floor based on distance
                 float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                 if      (b < 0.25)                          nShade =  '#'; // Very close
                 else if (b < 0.5)                           nShade =  'x';
                 else if (b < 0.75)                          nShade =  '.';
                 else if (b < 0.9)                           nShade =  '-';
                 else                                        nShade =  ' ';    // Too far away
                 screen[y*nScreenWidth + x] = nShade;
            }
        }
    }

    // Display stats
    swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);
    
    // Display map
    for (int nx = 0; nx < nMapWidth; nx++){
        for (int ny = 0; ny < nMapWidth; ny++){
            // Go through coordinates of map and put in screen buffer. Offset
            // by 1 so we don't overwrite stats  
            screen[(ny + 1)*nScreenWidth + nx] = map[ny*nMapWidth + nx];
        }
    }   
    
    // show player on map
    screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';


    // Write to the screen
    screen[nScreenWidth*nScreenHeight -1] = '\0'; 
                                // handle, array, how many bytes, coordinates, #bytes
    WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, {0,0}, &dwBytesWritten);
    }

	return 0;
}
