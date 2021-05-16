#include <iostream>
using namespace std;

#include "olcConsoleGameEngine.h"

class FormulaOLC : public olcConsoleGameEngine
{
public:
	FormulaOLC()
	{
		m_sAppName = L"Formula Racing";

	}
private:
	float fDistance = 0.0f;			// Distance car has travelled around track
	float fCurvature = 0.0f;		// Current track curvature, lerped between track sections
	float fTrackCurvature = 0.0f;	// Accumulation of track curvature
	float fTrackDistance = 0.0f;	// Total distance of track

	float fCarPos = 0.0f;			// Current car position
	float fPlayerCurvature = 0.0f;			// Accumulation of player curvature
	float fSpeed = 0.0f;			// Current player speed

	vector<pair<float, float>> vecTrack; // Track sections, sharpness of bend, length of section

	list<float> listLapTimes;			// List of previous lap times
	float fCurrentLapTime;			// Current lap time




protected:
	// override
	virtual bool OnUserCreate()
	{

		// Define track
		vecTrack.push_back(make_pair(0.0f, 10.0f));
		vecTrack.push_back(make_pair(0.0f, 200.0f));
		vecTrack.push_back(make_pair(1.0f, 200.0f));
		vecTrack.push_back(make_pair(0.0f, 400.0f));
		vecTrack.push_back(make_pair(-1.0f, 100.0f));
		vecTrack.push_back(make_pair(0.0f, 200.0f));
		vecTrack.push_back(make_pair(-1.0f, 200.0f));
		vecTrack.push_back(make_pair(1.0f, 200.0f));
		vecTrack.push_back(make_pair(0.0f, 200.0f));
		vecTrack.push_back(make_pair(0.2f, 500.0f));
		vecTrack.push_back(make_pair(0.0f, 200.0f));

		// Calculate total track distance
		for (auto t : vecTrack)
			fTrackDistance += t.second;


		return true;
	}

	// override
	virtual bool OnUserUpdate(float fElapsedTime)
	{
		// Control input
		int nCarDirection = 0;

		if (m_keys[VK_UP].bHeld)
			fSpeed += 2.0f * fElapsedTime;
		else
			fSpeed -= 1.0f * fElapsedTime;

		// Car Curvature is accumulated left/right input
		if (m_keys[VK_LEFT].bHeld)
		{
			fPlayerCurvature -= 0.7f * fElapsedTime * (1.0f - fSpeed / 2.0f);
			nCarDirection = -1;
		}

		if (m_keys[VK_RIGHT].bHeld)
		{
			fPlayerCurvature += 0.7f * fElapsedTime * (1.0f - fSpeed / 2.0f);
			nCarDirection = +1;
		}

		// If car curvature is too different to track curvature, slow down
		// as car has gone off track
		if (fabs(fPlayerCurvature - fTrackCurvature) >= 0.8f)
			fSpeed -= 5.0f * fElapsedTime;


		// Speed clamp
		if (fSpeed < 0.0f) fSpeed = 0.0f;
		if (fSpeed > 1.0f) fSpeed = 1.0f;

		// Move car
		fDistance += (70.0f * fSpeed) * fElapsedTime;


		float fOffset = 0;
		float nTrackSection = 0;

		fCurrentLapTime += fElapsedTime;

		// Make the track a loop
		if (fDistance >= fTrackDistance) {
			// a new lap
			fDistance -= fTrackDistance;
			listLapTimes.push_front(fCurrentLapTime);
			fCurrentLapTime = 0.0f;
		}

		// Find position on track
		while (nTrackSection < vecTrack.size() && fOffset <= fDistance) {
			fOffset += vecTrack[nTrackSection].second;
			nTrackSection++;
		}

		// Adjust Curvature
		float fTargetCurvature = vecTrack[nTrackSection - 1].first;
		float fTrackCurvatureDiff = (fTargetCurvature - fCurvature) * fElapsedTime * fSpeed;

		// Accumulate player curvature
		fCurvature += fTrackCurvatureDiff;

		// Accumulate track curvature
		fTrackCurvature += (fCurvature)*fElapsedTime * fSpeed;

		// Draw Background
		// Sky
		for (int y = 0; y < ScreenHeight() / 2; y++)
			for (int x = 0; x < ScreenWidth(); x++)
				Draw(x, y, y < ScreenHeight() / 4 ? PIXEL_HALF : PIXEL_SOLID, FG_DARK_BLUE);

		// Scenery
		for (int x = 0; x < ScreenWidth(); x++)
		{
			int nHillHeight = (int)(fabs(sinf(x * 0.01f + fTrackCurvature) * 16.0f));
			for (int y = (ScreenHeight() / 2) - nHillHeight; y < ScreenHeight() / 2; y++)
				Draw(x, y, PIXEL_SOLID, FG_DARK_YELLOW);
		}

		// Track
		for (int y = 0; y < ScreenHeight() / 2; y++) {
			for (int x = 0; x < ScreenWidth(); x++) {

				float fPerspective = (float)y / (ScreenHeight() / 2.0f);

				float fMiddlePoint = 0.5f + fCurvature * powf((1.0f - fPerspective), 3);
				float fRoadWidth = 0.1f + fPerspective * 0.80f;
				float fClipWidth = fRoadWidth * 0.15f;

				fRoadWidth *= 0.5f;

				// Segment boundaries
				int nLeftGrass = (fMiddlePoint - fRoadWidth - fClipWidth) * ScreenWidth();
				int nLeftClip = (fMiddlePoint - fRoadWidth) * ScreenWidth();
				int nRightClip = (fMiddlePoint + fRoadWidth) * ScreenWidth();
				int nRightGrass = (fMiddlePoint + fRoadWidth + fClipWidth) * ScreenWidth();

				int nRow = ScreenHeight() / 2 + y;

				// Using periodic oscillatory functions to give lines, where the phase is controlled
				// by the distance around the track.
				int nGrassColour = sinf(20.0f * powf(1.0f - fPerspective, 3) + fDistance * 0.1f) > 0.0f ? FG_GREEN : FG_DARK_GREEN;
				int nClipColour = sinf(80.0f * powf(1.0f - fPerspective, 2) + fDistance) > 0.0f ? FG_RED : FG_WHITE;

				int nRoadColor = (nTrackSection - 1) == 0 ? FG_YELLOW : FG_GREY;

				// Draw the row segments
				if (x >= 0 && x < nLeftGrass) {
					Draw(x, nRow, PIXEL_SOLID, nGrassColour);
				}

				if (x >= nLeftGrass && x < nLeftClip) {
					Draw(x, nRow, PIXEL_SOLID, nClipColour);
				}

				if (x >= nLeftClip && x < nRightClip) {
					Draw(x, nRow, PIXEL_SOLID, nRoadColor);
				}

				if (x >= nRightClip && x < nRightGrass) {
					Draw(x, nRow, PIXEL_SOLID, nClipColour);
				}

				if (x >= nRightGrass && x < ScreenWidth()) {
					Draw(x, nRow, PIXEL_SOLID, nGrassColour);
				}


			}
		}

		// Draw Car
		fCarPos = fPlayerCurvature - fTrackCurvature;
		int nCarPos = ScreenWidth() / 2 + +((int)(ScreenWidth() * fCarPos) / 2.0f) - 7;

		// Draw a car that represents straight/left/right curves
		switch (nCarDirection)
		{
		case 0:
			DrawStringAlpha(nCarPos, 80, L"   ||####||   ");
			DrawStringAlpha(nCarPos, 81, L"      ##      ");
			DrawStringAlpha(nCarPos, 82, L"     ####     ");
			DrawStringAlpha(nCarPos, 83, L"     ####     ");
			DrawStringAlpha(nCarPos, 84, L"|||  ####  |||");
			DrawStringAlpha(nCarPos, 85, L"|||########|||");
			DrawStringAlpha(nCarPos, 86, L"|||  ####  |||");
			break;

		case +1:
			DrawStringAlpha(nCarPos, 80, L"      //####//");
			DrawStringAlpha(nCarPos, 81, L"         ##   ");
			DrawStringAlpha(nCarPos, 82, L"       ####   ");
			DrawStringAlpha(nCarPos, 83, L"      ####    ");
			DrawStringAlpha(nCarPos, 84, L"///  ####//// ");
			DrawStringAlpha(nCarPos, 85, L"//#######///O ");
			DrawStringAlpha(nCarPos, 86, L"/// #### //// ");
			break;

		case -1:
			DrawStringAlpha(nCarPos, 80, L"\\\\####\\\\      ");
			DrawStringAlpha(nCarPos, 81, L"   ##         ");
			DrawStringAlpha(nCarPos, 82, L"   ####       ");
			DrawStringAlpha(nCarPos, 83, L"    ####      ");
			DrawStringAlpha(nCarPos, 84, L" \\\\\\\\####  \\\\\\");
			DrawStringAlpha(nCarPos, 85, L" O\\\\\\#######\\\\");
			DrawStringAlpha(nCarPos, 86, L" \\\\\\\\ #### \\\\\\");
			break;
		}

		// Display Stats
		DrawString(0, 0, L"Distance: " + to_wstring(fDistance));
		DrawString(0, 1, L"Target Curvature: " + to_wstring(fCurvature));
		DrawString(0, 2, L"Player Curvature: " + to_wstring(fPlayerCurvature));
		DrawString(0, 3, L"Player Speed    : " + to_wstring(fSpeed));
		DrawString(0, 4, L"Track Curvature : " + to_wstring(fTrackCurvature));

		auto disp_time = [](float t)
		{
			int nMinutes = t / 60.0f;
			int nSeconds = t - (nMinutes * 60.0f);
			int nMilliSeconds = (t - (float)nSeconds) * 1000.0f;
			return to_wstring(nMinutes) + L"." + to_wstring(nSeconds) + L":" + to_wstring(nMilliSeconds);
		};

		// Display current laptime
		DrawString(10, 8, disp_time(fCurrentLapTime));

		// Display last lap times
		int j = 10;
		for (auto l : listLapTimes)
		{
			DrawString(10, j, disp_time(l));
			j++;
		}

		return true;

	}
};
int main() {

	FormulaOLC game;
	game.ConstructConsole(160, 100, 8, 8);
	game.Start();
	return 0;
}