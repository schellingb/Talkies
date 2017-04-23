/*
  Talkies
  Copyright (C) 2017 Bernhard Schelling

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <ZL_Application.h>
#include <ZL_Display.h>
#include <ZL_Surface.h>
#include <ZL_Signal.h>
#include <ZL_Audio.h>
#include <ZL_Font.h>
#include <ZL_Scene.h>
#include <ZL_Input.h>
#include <ZL_SynthImc.h>
#include <vector>
#include <algorithm>
using namespace std;

extern ZL_Sound sndError, sndClick, sndConnect;
extern ZL_SynthImcTrack sndBuild, sndSong;

static const ZL_Color BodyColors[] = { ZLRGBX(0xF6D9CB), ZLRGBX(0xEFC0A4), ZLRGBX(0xD68D6A), ZLRGBX(0xEDB886), ZLRGBX(0xC98558), ZLRGBX(0x5B3C28) };
static const ZL_Color HairColors[] = { ZLRGBX(0x090806), ZLRGBX(0x2C222B), ZLRGBX(0x3B302A), ZLRGBX(0x4E433F), ZLRGBX(0x504444), ZLRGBX(0x6D4D40), ZLRGBX(0x554838), ZLRGBX(0xA7856A), ZLRGBX(0xB8967A), ZLRGBX(0xDBD1B8), ZLRGBX(0xDFBA9D), ZLRGBX(0xA5856C), ZLRGBX(0xB89778), ZLRGBX(0xDCD0BA), ZLRGBX(0xDEBC99), ZLRGBX(0x977961), ZLRGBX(0xE6CEA8), ZLRGBX(0xE3C8AA), ZLRGBX(0xA56B46), ZLRGBX(0x91553D), ZLRGBX(0x533D32), ZLRGBX(0x71635A), ZLRGBX(0xB7A69E), ZLRGBX(0xD6C4C2), ZLRGBX(0xFFF5E1), ZLRGBX(0xCBBFB1), ZLRGBX(0x8D4A42), ZLRGBX(0xB6523A) };
static const ZL_Color ShoeColors[] = { ZLRGBX(0xFFFFFF), ZLRGBX(0x666666), ZLRGBX(0x000000), ZLRGBX(0x995E1E), ZLRGBX(0x531F00), ZLRGBX(0x4E004E), ZLRGBX(0x000033), ZLRGBX(0x005A1A), ZLRGBX(0x8D0000) };

static const ZL_Color Color_Connected        =  ZLRGB(.1,1,.3); //;ZL_Color::Green;
static const ZL_Color Color_Unconnected      =  ZLRGB(.2,.6,.9);
static const ZL_Color Color_UnconnectedAlpha = ZLRGBA(.2,.6,.9,.5);
static const ZL_Color Color_Focus            = ZLRGBA(1,.8,.2,.5);

static ZL_Surface srfChars, srfThink, srfExchange, srfBeam, srfLudumDare, srfPlanetWater, srfPlanetEarth, srfPlanetEarthOut;
static ZL_Font fntMain;
static ZL_Polygon polyIn, polyOut, polySea;

static const scalar WorldZoom = 400.f;
static scalar WorldRot = 0.f;

static bool ConnectionDrawing;
static size_t ConnectionsRequired, ConnectionsSet;

static const unsigned int SeedPlanetLevels[] = { 944335, 291046, 655273, 510192, 512481, 635711, 699340, 577545, 654235, 1 };
static const unsigned int SeedPeopleLevels[] = {  32592, 874176, 823181, 687927, 611694, 126251, 558624, 921386, 442962, 1 };
static size_t Level = 0;

static float FadeStart;
static bool FadeIn;
static enum eFadeMode { FADE_NONE, FADE_STARTUP, FADE_TOGAME, FADE_NEXTLEVEL, FADE_BACKTOTITLE, FADE_QUIT } FadeMode;
enum { INPUTLOCK_NONE, INPUTLOCK_FADE };

static vector<ZL_Vector> points;
static vector<ZL_Vector> seapoints[50];
static size_t seacount = 0;

enum eTutorialStep  { TUT_NONE, TUT_01_WELCOME, TUT_02_EXPLAIN, TUT_03_CONNECT, TUT_04_GONEXT, TUT_05_CONNECT, TUT_06_EXCHANGE, TUT_07_CONNECT, TUT_08_DONE, TUT_X_LEVELDONE, TUT_X_GAMEDONE };
static const bool TutorialClick[] = { 0,     1,              1,              0,             1,              0,               0,              0,           1,               1,              1 };
static const char* TutorialText[] = { 0,
	"Hello!\nWelcome to TALKIES a game about a small world",
	"The goal of this game is to connect people!",
	"On this small world     wants to talk to      \nClick and hold the person and draw a connection!",
	"Well done! Lets go to the next world!",
	"Here     wants to talk to      \nMake it happen!",
	"Now the next connection is not possible, but we can help\nClick and hold between two neighbors to switch their place.",
	"Now you can connect      and      !\n",
	"Well done! Thats the tutorial! Enjoy!",
	"Stage Cleared!",
	"Game Finished!\nThank you for playing!!",
};
static size_t TutorialNum = TUT_01_WELCOME;

struct sChar
{
	int      IdxBody, IdxPants, IdxShoes, IdxShirt, IdxHair, IdxHat, IdxFace;
	ZL_Color ColBody, ColPants, ColShoes, ColShirt, ColHair, ColHat;
	
	sChar()
	{
		IdxBody = 0;
		IdxPants = RAND_INT_RANGE(0, 4);
		IdxShoes = RAND_INT_RANGE(0, 3);
		IdxShirt = RAND_INT_RANGE(0, 3);
		IdxHair  = RAND_INT_RANGE(0, 6);
		IdxHat   = RAND_INT_RANGE(0, 2+10);
		IdxFace  = (RAND_CHANCE(2) ? RAND_INT_RANGE(1, 5) : 0);
		ColBody  = RAND_ARRAYELEMENT(BodyColors);
		ColPants = RAND_COLOR;
		ColShoes = (RAND_CHANCE(5) ? RAND_COLOR : RAND_ARRAYELEMENT(ShoeColors));
		ColShirt = RAND_COLOR;
		ColHair  = (RAND_CHANCE(10) ? RAND_BRIGHTCOLOR : RAND_ARRAYELEMENT(HairColors));
		ColHat   = (RAND_CHANCE(5) ? RAND_COLOR : RAND_ARRAYELEMENT(ShoeColors));
	}

	void DrawAt(ZL_Vector& p, float a)
	{
		srfChars.SetRotate(a);
		srfChars.SetTilesetIndex(IdxBody,  0).Draw(p, ColBody );
		srfChars.SetTilesetIndex(IdxPants, 1).Draw(p, ColPants);
		srfChars.SetTilesetIndex(IdxShoes, 2).Draw(p, ColShoes);
		srfChars.SetTilesetIndex(IdxShirt, 3).Draw(p, ColShirt);
		srfChars.SetTilesetIndex(IdxFace,  6).Draw(p);
		srfChars.SetTilesetIndex(IdxHair,  4).Draw(p, ColHair );
		srfChars.SetTilesetIndex(IdxHat,   5).Draw(p + ZLV(0, 16).Rotate(a), ColHat  );
	}
};

struct sPerson : public sChar
{
	ZL_Vector Pos;
	float Angle;
	size_t Id;
	sPerson *Target, *Source, *Left, *Right;
	bool TargetConnected;

	sPerson(const ZL_Vector& InPos, float InAngle, size_t InId) : Pos(InPos), Angle(InAngle), Id(InId), Target(NULL), Source(NULL) //, TargetLineDrawing(false)
	{
	}

	ZL_Vector GetDrawPos()
	{
		ZL_Vector p = Pos;
		if      (Target)                            p += ZL_Vector::FromAngle(Angle+PIHALF) * ZL_Math::Max(0.f, 10.f*ssin(ZLTICKS*0.01f+Angle));
		else if (Source && Source->TargetConnected) p += ZL_Vector::FromAngle(Angle+PIHALF) * ZL_Math::Max(0.f, 10.f*ssin(ZLTICKS*0.01f+Source->Angle));
		return p;
	}

	void Draw()
	{
		ZL_Vector p = GetDrawPos();
		DrawAt(p, Angle);
		if (Target)
		{
			srfThink.Draw(p + ZLV(-4.f,         60.f).Rotate(Angle), Angle, (TargetConnected ? Color_Connected : ZLWHITE));
			Target->DrawAt(p + ZLV(-1, 70).Rotate(Angle) - ZLV(0, 16.f).Rotate(-WorldRot), -WorldRot);
		}
	}

	void DrawHighlighted(const ZL_Color& col)
	{
		ZL_Vector p = GetDrawPos();
		srfChars.SetTilesetIndex(0,  7);
		for (int i = 0; i < 9; i++) srfChars.Draw(p.x + (i/3-1)*2.f, p.y + (i%3-1)*2.f, Angle, col);
	}
};

struct sConnection
{
	vector<ZL_Vector> TargetLine;
	ZL_Polygon TargetLinePoly;
	sPerson *From, *To;
	sConnection() { ClearTargetLine(); }

	void Draw()
	{
		srfBeam.SetColor(To ? Color_Connected : Color_Unconnected);
		TargetLinePoly.Draw();
	}

	void ClearTargetLine(sPerson *SetFrom = NULL)
	{
		TargetLine.clear();
		TargetLinePoly.Clear();
		ConnectionDrawing = false;
		From = SetFrom;
		To = NULL;
	}

	void SetConnected(bool State)
	{
		if (From && From->TargetConnected != State) ConnectionsSet = ConnectionsSet + (State ? 1 : -1);
		if (From) From->TargetConnected = State;
		if (To) To->TargetConnected = State;
	}

	void AddTargetLine(const ZL_Vector& p, sPerson *DoConnect)
	{
		To = DoConnect;
		ConnectionDrawing = !DoConnect;
		if (TargetLine.empty()) TargetLine.push_back(From->Pos);
		TargetLine.push_back(p);
		RefreshTargetLine();
	}

	void RefreshTargetLine()
	{
		if (TargetLine.empty()) return;
		if (!TargetLinePoly) TargetLinePoly = ZL_Polygon(srfBeam);
		TargetLine[0] = From->Pos;
		if (To) TargetLine.back() = To->Pos;
		TargetLinePoly.Clear();
		TargetLinePoly.Extrude(TargetLine, 2.f, -2.f, true, false);
	}

	bool CrossesTargetLine(const ZL_Vector& a, const ZL_Vector& b)
	{
		if (!TargetLine.empty() && ZL_Math::LineCollision(From->Pos, TargetLine[0], a, b)) return true;
		for (size_t i = 1; i < TargetLine.size(); i++)
			if (ZL_Math::LineCollision(TargetLine[i-1], TargetLine[i], a, b)) return true;
		return (To ? ZL_Math::LineCollision(TargetLine.back(), To->Pos, a, b) : false);
	}

	bool CrossesOtherConnection(sConnection* other)
	{
		if (!TargetLine.empty() && other->CrossesTargetLine(From->Pos, TargetLine[0])) return true;
		for (size_t i = 1; i < TargetLine.size(); i++)
			if (other->CrossesTargetLine(TargetLine[i-1], TargetLine[i])) return true;
		return (To ? other->CrossesTargetLine(TargetLine.back(), To->Pos) : false);
	}
};

static vector<sChar> TitleChars;
static vector<sPerson> People;
static vector<sConnection> Connections;
static sConnection DrawConnection;
static sPerson *pDragSource;

static bool GoesOut(const ZL_Vector& a, const ZL_Vector& b)
{
	for (size_t i = 1; i < points.size(); i++) if (ZL_Math::LineCollision(points[i-1], points[i], a, b)) return true;
	if (ZL_Math::LineCollision(points[0], points.back(), a, b)) return true;
	return false;
}

static void StartGame()
{
	unsigned int SeedPlanet = SeedPlanetLevels[Level], SeedPeople = SeedPeopleLevels[Level];

	#ifdef ZILLALOG
	if (ZL_Display::KeyDown[ZLK_LSHIFT])
	{
		SeedPlanet = RAND_INT_RANGE(0, 999999);
		SeedPeople = RAND_INT_RANGE(0, 999999);
	}
	ZL_LOG("GAME", "Level: %d - Seed Planet: %u - People: %u", Level, SeedPlanet, SeedPeople);
	#endif
	ZL_SeededRand RandPlanet(SeedPlanet), RandPeople(SeedPeople);

	static int ticks = 64;
	scalar Radius = 100, Roughness = s(0.6), hmmin = 0, hmmax = 0, extend = s(1.0), ElevationSize = 60, Rot = 0;//RAND_FACTOR*PI2;
	vector<scalar> hm(ticks+1);
	hm[0] = hm[ticks] = 0;
	for (int step = ticks; step != 1; step>>=1, extend *= Roughness) for (int half = step>>1, i = half; i < ticks; i += step)
	{
		hm[i] = ((hm[i-half] + hm[i+half]) / 2) + (RandPlanet.Factor()*2*extend);
		if (hm[i] < hmmin) hmmin = hm[i]; else if (hm[i] > hmmax) hmmax = hm[i];
	}
	for (int i = 0; i < ticks+1; i++) { hm[i] = Radius+ElevationSize*((hm[i]-hmmin)/(hmmax-hmmin)); }

	points.clear();
	for (int i = 0; i < ticks; i++)
	{
		float a = Rot+PI2*s(i)/ticks, len = hm[i];
		points.push_back(ZL_Vector::FromAngle(a).Mul(len));
	}

	float searanges[50][2];
	ZL_Polygon::PointList* pointlists[51] = { &points };
	seacount = 0;
	for (size_t seatarget = RandPlanet.Int(2,5); seatarget; seatarget--)
	{
		RetrySea:
		float start = RandPlanet.Angle(), end = start + (RandPlanet.Range(10, 30)*PIOVER180);
		for (size_t i = 0; i < seacount; i++)
			if ((end > searanges[i][0]-.1f     && start < searanges[i][1]+.1f    ) ||
				(end > searanges[i][0]-.1f-PI2 && start < searanges[i][1]+.1f-PI2) ||
				(end > searanges[i][0]-.1f+PI2 && start < searanges[i][1]+.1f+PI2)) goto RetrySea;

		size_t a = (size_t)(start * ticks / PI2) + 1, b = (size_t)(end * ticks / PI2);
		float MinLenSq = S_MAX, LenSq;
		for (size_t j = a + points.size() - 2; j <= b + points.size() + 2; j++) if ((LenSq = points[j % points.size()].GetLengthSq()) < MinLenSq) MinLenSq = LenSq;
		float seasurface = ssqrt(LenSq) - 5.f, seafloor = seasurface - RandPlanet.Range(10,50);
		for (size_t j = a; j <= b; j++) points[j % points.size()].SetLength(seasurface);

		seapoints[seacount].clear();
		seapoints[seacount].push_back(ZL_Vector::FromAngle(start+Rot) * 1000);
		seapoints[seacount].push_back(ZL_Vector::FromAngle(end  +Rot) * 1000);
		seapoints[seacount].push_back(ZL_Vector::FromAngle(end  +Rot) * seafloor);
		seapoints[seacount].push_back(ZL_Vector::FromAngle(start+Rot) * seafloor);
		searanges[seacount][0] = start;
		searanges[seacount][1] = end;
		pointlists[1+seacount] = &seapoints[seacount];
		seacount++;
	}

	People.clear();
	for (float a = 0; a < PI2; a += 0.2f)
	{
		bool onsea = false;
		for (size_t i = 0; i < seacount; i++)
			if ((a > searanges[i][0]-.1f     && a < searanges[i][1]+.1f    ) ||
				(a > searanges[i][0]-.1f-PI2 && a < searanges[i][1]+.1f-PI2)) { onsea = true; break; }
		if (onsea) continue;

		size_t i1 = (size_t)(a * ticks / PI2), i2 = (i1 + 1) % ticks;
		ZL_Vector Pos = points[i1].VecLerp(points[i2], (a * ticks / PI2) - i1);

		ZL_Vector Normal;
		for (int j = -2; j <= 3; j++) Normal += points[(i1+ticks+j)%ticks];
		People.push_back(sPerson(Pos, Normal.GetAngle()-PIHALF, People.size()));
	}

	for (size_t i = 0; i < People.size(); i++)
	{
		sPerson *pl = &People[i], *pr = &People[(i + 1) % People.size()];
		float al = pl->Pos.GetAngle(), ar = al + ZL_Math::RelAngle(al, pr->Pos.GetAngle());
		bool hassea = false;
		for (size_t i = 0; i < seacount; i++)
			if ((searanges[i][0] > al     && searanges[i][0] < ar    ) ||
				(searanges[i][0] > al-PI2 && searanges[i][0] < ar-PI2)) { hassea = true; break; }
		pl->Right = (hassea ? NULL : pr);
		pr->Left  = (hassea ? NULL : pl);
	}

	polySea = ZL_Polygon(srfPlanetWater.SetTextureRepeatMode().SetScale(0.3f).SetColor(ZLRGBX(0x6990FF))).Add(pointlists, seacount+1, (ZL_Polygon::IntersectMode)4);
	reverse(points.begin(), points.end());
	polyIn = ZL_Polygon(srfPlanetEarth.SetTextureRepeatMode().SetScale(0.3f).SetColor(ZLRGBX(0xDFBC89))).Add(pointlists, seacount+1, (ZL_Polygon::IntersectMode)3);
	std::vector<ZL_Vector> SurfacePoints;
	ZL_Polygon::GetBorder(pointlists, seacount+1, SurfacePoints, (ZL_Polygon::IntersectMode)3);
	polyOut = ZL_Polygon(srfPlanetEarthOut.SetTextureRepeatMode().SetScale(0.3f)).Extrude(SurfacePoints, 8.f, -4.f, true);
	points = SurfacePoints;

	ConnectionsSet = 0;
	ConnectionsRequired = RandPeople.Int(1,5);
	for (size_t i = 0; i < ConnectionsRequired; i++)
	{
		sPerson *Source, *Target;
		do { Source = &RandPeople.Element(People); } while (Source->Target || Source->Source);
		do { Target = &RandPeople.Element(People); } while (Target->Target || Target->Source || Source->Left == Target || Source->Right == Target || Source == Target);
		Source->Target = Target;
		Source->TargetConnected = false;
		Target->Source = Source;
	}

	Connections.clear();
	DrawConnection = sConnection();
	ConnectionDrawing = false;

	if (TutorialNum == TUT_04_GONEXT) TutorialNum++;
	if (TutorialNum == TUT_08_DONE) TutorialNum = TUT_NONE;
	if (TutorialNum == TUT_X_LEVELDONE) TutorialNum = TUT_NONE;
	if (TutorialNum && Level == 0) WorldRot = 2.3f;
	if (TutorialNum && Level == 1) WorldRot = 1.0f;
}

static void StartTitle()
{
	for (size_t i = 0; i < 100; i++)
		TitleChars.push_back(sChar());
}

static void DrawOutlineText(scalar x, scalar y, scalar out, const char* txt, scalar scale, ZL_Origin::Type origin = ZL_Origin::Center)
{
	for (int i = 0; i < 9; i++) fntMain.Draw(x + (i/3-1)*out, y + (i%3-1)*out, txt, scale, ZL_Color::Black, origin);
	fntMain.Draw(x, y, txt, scale, origin);
}

void FadeTo(eFadeMode NewFadeMode)
{
	if (FadeMode) return;
	FadeMode = NewFadeMode;
	ZL_Input::SetLock(INPUTLOCK_FADE);
	FadeStart = -1;
	FadeIn = (FadeMode == FADE_STARTUP);
}

void DrawTitle()
{
	float cy = 200*ZL_Display::Height/ZL_Display::Width;
	ZL_Display::FillGradient(0, 0, ZLWIDTH, ZLHEIGHT, ZLRGB(0,.1,0), ZLRGB(0,.1,0), ZLRGB(.1,.8,.1), ZLRGB(.1,.5,.1));
	ZL_Display::PushOrtho(0, 400, 0, 400*ZL_Display::Height/ZL_Display::Width);
	for (size_t i = 0; i < TitleChars.size(); i++)
		TitleChars[i].DrawAt(ZLV(  (i%10)*400/10.f+((i/10)%2?30:10), cy+cy-32-(i/10)*cy/5.f + ZL_Math::Max(0.f, 10.f*ssin(ZLTICKS*0.01f+i))), 0);
	ZL_Vector TitlePos = ZLV(200 + scos(ZLSECONDS)*10.f, cy + ssin(ZLSECONDS)*10.f);
	DrawOutlineText(TitlePos.x, TitlePos.y, 2.f, "TALKIES", .5f,  ZL_Origin::Center);
	ZL_Display::PopOrtho();

	DrawOutlineText(ZLHALFW, ZLHALFH-150.f, 3.f, "CLICK TO START", .3f);
	DrawOutlineText(ZLHALFW, ZLHALFH-250.f, 2.f, "Press Alt-Enter for fullscreen", .2f);
	DrawOutlineText(25.f, 17.f, 2.f, "c 2017 Bernhard Schelling - Nukular Design", .3f, ZL_Origin::TopLeft);
	srfLudumDare.Draw(ZLFROMW(10), 10);

	if (ZL_Input::Clicked())
	{
		if (TutorialNum > TUT_NONE && TutorialNum < TUT_X_LEVELDONE) { TutorialNum = TUT_01_WELCOME; Level = 0; }
		//DEBUG SKIP TO LEVEL: TutorialNum = TUT_NONE; Level = 6;
		FadeTo(FADE_TOGAME);
	}
	if (ZL_Input::Up(ZLK_ESCAPE)) FadeTo(FADE_QUIT);
}

void DrawGame()
{
	WorldRot += ZLELAPSEDF(0.02);
	ZL_Display::ClearFill(ZLRGBX(0x0B0B63));
	ZL_Display::PushOrtho(-WorldZoom, WorldZoom, -WorldZoom*ZL_Display::Height/ZL_Display::Width, WorldZoom*ZL_Display::Height/ZL_Display::Width);
	ZL_Display::Rotate(WorldRot);
	const ZL_Vector MousePos = ZL_Display::ScreenToWorld(ZL_Display::PointerPos());

	sPerson *pFocus = NULL;
	if (!TutorialClick[TutorialNum])
	{
		float FocusDistSq = S_MAX, DistSq;
		for (vector<sPerson>::iterator it = People.begin(); it != People.end(); ++it)
			if ((DistSq = MousePos.GetDistanceSq(it->Pos)) < FocusDistSq) { FocusDistSq = DistSq; pFocus = &*it; }
		if (TutorialNum)
		{
			if (TutorialNum == TUT_03_CONNECT  && (pFocus->Id !=  3 && pFocus->Id !=  8)) pFocus = NULL;
			if (TutorialNum == TUT_05_CONNECT  && (pFocus->Id !=  9 && pFocus->Id != 15)) pFocus = NULL;
			if (TutorialNum == TUT_06_EXCHANGE && (pFocus->Id != 14 && pFocus->Id != 15)) pFocus = NULL;
			if (TutorialNum == TUT_07_CONNECT  && (pFocus->Id != 14 && pFocus->Id != 22)) pFocus = NULL;
		}

		if (ZL_Input::Down() && pFocus)
		{
			ZL_LOG("GAME", "Person: %u - TutorialNum: %u", pFocus->Id, TutorialNum);
			pDragSource = pFocus;
		}
		if (ZL_Input::Held() && pDragSource)
		{
			const ZL_Vector Prev = (ConnectionDrawing ? DrawConnection.TargetLine.back() : pDragSource->Pos - pDragSource->Pos.VecNorm());
			if (ConnectionDrawing && (pDragSource->Target == pFocus || pDragSource->Source == pFocus) && FocusDistSq < 10*10 && pFocus)
			{
				DrawConnection.AddTargetLine(pFocus->Pos, pFocus);
				DrawConnection.SetConnected(true);
				Connections.push_back(DrawConnection);
				DrawConnection = sConnection();
				pDragSource = NULL;
				if (TutorialNum == TUT_03_CONNECT || TutorialNum == TUT_05_CONNECT || TutorialNum == TUT_07_CONNECT) TutorialNum++;
				size_t UnconnectedCount = 0;
				for (vector<sPerson>::iterator it = People.begin(); it != People.end(); ++it) 
					if (it->Target && !it->TargetConnected) UnconnectedCount++;
				if (TutorialNum == TUT_NONE && UnconnectedCount == 0) 
					TutorialNum = (Level == COUNT_OF(SeedPlanetLevels)-1 ? TUT_X_GAMEDONE : TUT_X_LEVELDONE);
			}
			else if (MousePos.GetDistanceSq(Prev) < 10*10)
			{

			}
			else if (!GoesOut(Prev, MousePos) && TutorialNum != TUT_06_EXCHANGE)
			{
				if (!ConnectionDrawing)
				{
					for (vector<sConnection>::iterator it = Connections.begin(); it != Connections.end();)
						if (it->From == pDragSource || it->To == pDragSource) { it->SetConnected(false); it = Connections.erase(it); }
						else ++it;
					DrawConnection.ClearTargetLine(pDragSource);
				}

				for (vector<sConnection>::iterator it = Connections.begin(); it != Connections.end();)
				{
					if (!it->CrossesTargetLine(Prev, MousePos)) { ++it; continue; }
					DrawConnection.ClearTargetLine();
					pDragSource = NULL;
					break;
				}
				if (DrawConnection.From)
				{
					DrawConnection.AddTargetLine(MousePos, false);
				}
			}
		}
		if (ZL_Input::Up())
		{
			if (pDragSource && ConnectionDrawing)
			{
				DrawConnection.ClearTargetLine();
			}
			else if (pDragSource && pFocus && (pDragSource->Left == pFocus || pDragSource->Right == pFocus))
			{
				swap(pDragSource->Pos, pFocus->Pos);
				swap(pDragSource->Angle, pFocus->Angle);
				sPerson* Chain[4];
				if (pDragSource->Left  == pFocus) { Chain[0] = pFocus->Left, Chain[1] = pDragSource, Chain[2] = pFocus, Chain[3] = pDragSource->Right; }
				if (pDragSource->Right == pFocus) { Chain[0] = pDragSource->Left, Chain[1] = pFocus, Chain[2] = pDragSource, Chain[3] = pFocus->Right; }
				for (size_t i = 0; i < 4; i++)
				{
					if (Chain[i] && i > 0) Chain[i]->Left  = Chain[i-1];
					if (Chain[i] && i < 3) Chain[i]->Right = Chain[i+1];
				}

				for (size_t i = 0; i < Connections.size(); i++)
				{
					sConnection* it = &Connections[i];
					if (it->From != pDragSource && it->To != pDragSource && it->From != pFocus && it->To != pFocus) continue;
					bool Crossing = false;
					for (size_t j = Connections.size(); --j > i;)
						if (it->CrossesOtherConnection(&Connections[j]))
							{ Connections[j].SetConnected(false); Connections.erase(Connections.begin() + j); Crossing = true; }
					if (Crossing) { it->SetConnected(false); Connections.erase(Connections.begin() + i--); }
					else it->RefreshTargetLine();
				}

				if (TutorialNum == TUT_06_EXCHANGE) TutorialNum++;
			}
			pDragSource = NULL;
		}
	}
	if (ZL_Input::Up(ZLK_ESCAPE)) FadeTo(FADE_BACKTOTITLE);

	polyIn.Draw();
	polyOut.Draw();
	polySea.Draw();

	#ifdef ZILLALOG
	if (ZL_Display::KeyDown[ZLK_LSHIFT])
	{
		for (size_t i = 0; i < points.size(); i++) ZL_Display::FillCircle(points[i], 1, ZL_Color::Red);
		ZL_Display::FillCircle(points[0], 5, ZL_Color::Red);
		for (size_t i = 0; i < points.size(); i++) 
			ZL_Display::DrawLine(points[(i+points.size()-1)%points.size()], points[i], ZL_Color::White);
		for (size_t i = 0; i < seacount; i++)
			for (size_t j = 0; j < 4; j++)
					ZL_Display::DrawLine(seapoints[i][(j+4-1)%4], seapoints[i][j], ZL_Color::Cyan);
		for (vector<sPerson>::iterator it = People.begin(); it != People.end(); ++it) 
			if (it->Target) it->DrawHighlighted(ZL_Color::Green); else if (it->Source) it->DrawHighlighted(ZL_Color::Red);
		if (ZL_Input::Up(ZLK_SPACE)) StartGame();
	}
	#endif

	bool CanExchange = (ZL_Input::Held() && pDragSource && !ConnectionDrawing && pFocus && (pDragSource->Left == pFocus || pDragSource->Right == pFocus));
	if (CanExchange) { pDragSource->DrawHighlighted(ZLRGBA(1,1,1,.3)); pFocus->DrawHighlighted(ZLRGBA(1,1,1,.3)); }
	else if (pFocus && !pDragSource) pFocus->DrawHighlighted(Color_Focus);
	else if (pDragSource) pDragSource->DrawHighlighted(Color_UnconnectedAlpha);

	for (vector<sPerson>::iterator it = People.begin(); it != People.end(); ++it) it->Draw();

	for (vector<sConnection>::iterator it = Connections.begin(); it != Connections.end(); ++it) it->Draw();
	DrawConnection.Draw();

	if (CanExchange)
	{
		float ExchangeAngle = pDragSource->Angle + ZL_Math::RelAngle(pDragSource->Angle, pFocus->Angle) * .5f;
		srfExchange.Draw(ZL_Vector::Lerp(pDragSource->Pos, pFocus->Pos, .5f) + ZL_Vector::FromAngle(ExchangeAngle + PIHALF) * 20.f, ExchangeAngle);
	}

	ZL_Display::PopOrtho();

	DrawOutlineText(10, 10, 2.f, ZL_String::format("Stage: %d/%d\nConnected: %d/%d", Level+1, COUNT_OF(SeedPlanetLevels), ConnectionsSet, ConnectionsRequired), .25, ZL_Origin::BottomLeft);

	if (TutorialNum)
	{
		ZL_Rectf RecTut(50, ZLFROMH(200), ZLFROMW(50), ZLFROMH(50));
		ZL_Display::DrawRect(RecTut, ZL_Color::Black, ZLLUMA(.5,.5));
		DrawOutlineText(ZLHALFW, ZLFROMH(125), 2.f, TutorialText[TutorialNum], .25f, ZL_Origin::Center);
		if (TutorialClick[TutorialNum]) DrawOutlineText(ZLFROMW(60), ZLFROMH(190), 2.f, "Click to Continue", .15f, ZL_Origin::BottomRight);

		if (TutorialClick[TutorialNum] && ZL_Input::Clicked(RecTut))
		{
			if (TutorialNum == TUT_04_GONEXT)   { TutorialNum--; FadeTo(FADE_NEXTLEVEL); }
			if (TutorialNum == TUT_08_DONE)     { TutorialNum--; FadeTo(FADE_NEXTLEVEL); }
			if (TutorialNum == TUT_X_LEVELDONE) { TutorialNum--; FadeTo(FADE_NEXTLEVEL); }
			if (TutorialNum == TUT_X_GAMEDONE)  { TutorialNum--; FadeTo(FADE_BACKTOTITLE); }
			TutorialNum++;
		}
		if (TutorialNum == TUT_03_CONNECT || TutorialNum == TUT_05_CONNECT || TutorialNum == TUT_07_CONNECT)
		{
			sPerson *Talker = NULL, *TalkerUnconnected = NULL;
			for (vector<sPerson>::iterator it = People.begin(); it != People.end(); ++it) 
				if (it->Target) { Talker = &*it; if (!TalkerUnconnected && !it->TargetConnected) { TalkerUnconnected = &*it; } }
			if (TalkerUnconnected) Talker = TalkerUnconnected;
			Talker->DrawAt(        ZLV(ZLHALFW-(TutorialNum == TUT_03_CONNECT ?  45 : (TutorialNum == TUT_05_CONNECT ? 133 : -105)), ZLFROMH(100)-20), 0);
			Talker->Target->DrawAt(ZLV(ZLHALFW+(TutorialNum == TUT_03_CONNECT ? 290 : (TutorialNum == TUT_05_CONNECT ? 193 :  220)), ZLFROMH(100)-20), 0);
		}
	}
}

void Draw()
{
	if (!TitleChars.empty()) DrawTitle();
	else DrawGame();

	if (FadeMode)
	{
		if (FadeStart < 0) FadeStart = ZLSECONDS;
		float t = ZL_Math::Min((ZLSECONDS - FadeStart) * 3.f, 1.f);
		ZL_Display::FillRect(0, 0, ZLWIDTH, ZLHEIGHT, ZLLUMA(0, (FadeIn ? 1.f - t : t)));
		if (t == 1.f && !FadeIn)
		{
			FadeIn = true;
			FadeStart = ZLSECONDS;
			if (FadeMode == FADE_TOGAME)      { TitleChars.clear(); StartGame(); }
			if (FadeMode == FADE_NEXTLEVEL)   { Level++; StartGame(); }
			if (FadeMode == FADE_BACKTOTITLE) StartTitle();
			if (FadeMode == FADE_QUIT)        ZL_Application::Quit();
		}
		else if (t == 1.f)
		{
			if (FadeMode == FADE_STARTUP) sndSong.Play();
			FadeMode = FADE_NONE;
			ZL_Input::RemoveLock();
		}
		if (!FadeIn && FadeMode == FADE_TOGAME)      sndSong.SetSongVolume(25 + (int)((1.f-t) * 75.f));
		if ( FadeIn && FadeMode == FADE_BACKTOTITLE) sndSong.SetSongVolume(25 + (int)((    t) * 75.f));
		if (!FadeIn && FadeMode == FADE_QUIT)        sndSong.SetSongVolume( 0 + (int)((1.f-t) * 99.f));
	}
}

static struct sTalkies : public ZL_Application
{
	sTalkies() : ZL_Application(60) { }

	virtual void Load(int argc, char *argv[])
	{
		if (!ZL_Application::LoadReleaseDesktopDataBundle()) return;
		if (!ZL_Display::Init("Talkies", 1280, 720, ZL_DISPLAY_ALLOWRESIZEHORIZONTAL)) return;
		ZL_Display::ClearFill(ZL_Color::Black);
		ZL_Audio::Init();
		ZL_Input::Init();
		
		fntMain = ZL_Font("Data/vipond_chubby.ttf.zip", 120).SetCharSpacing(5);
		srfChars = ZL_Surface("Data/chars.png").SetTextureFilterMode().SetTilesetClipping(16, 8).SetOrigin(ZL_Origin::BottomCenter);
		srfThink = ZL_Surface("Data/think.png").SetTextureFilterMode().SetOrigin(ZL_Origin::Center);
		srfExchange = ZL_Surface("Data/exchange.png").SetTextureFilterMode().SetOrigin(ZL_Origin::Center).SetScale(.5f);
		srfBeam = ZL_Surface("Data/beam.png").SetTextureRepeatMode().SetScale(.5f);
		srfLudumDare = ZL_Surface("Data/ludumdare.png").SetDrawOrigin(ZL_Origin::BottomRight);
		srfPlanetWater = ZL_Surface("Data/planet_water.png");
		srfPlanetEarth = ZL_Surface("Data/planet_earth.png");
		srfPlanetEarthOut = ZL_Surface("Data/planet_earthout.png");

		StartTitle();
		FadeTo(FADE_STARTUP);
	}

	virtual void AfterFrame() { Draw(); }
} Talkies;

static const unsigned int IMCERROR_OrderTable[] = {0x000000001,};
static const unsigned char IMCERROR_PatternData[] = {0x50, 255, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
static const unsigned char IMCERROR_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCERROR_EnvList[] = {{ 0, 256, 130, 5, 19, 255, true, 255, },{ 0, 256, 523, 8, 16, 255, true, 255, },};
static TImcSongEnvelopeCounter IMCERROR_EnvCounterList[] = {{ 0, 0, 238 }, { -1, -1, 256 }, { 1, 0, 256 },};
static const TImcSongOscillator IMCERROR_OscillatorList[] = {{ 5, 227, IMCSONGOSCTYPE_SQUARE, 0, -1, 100, 1, 1 },{ 6, 0, IMCSONGOSCTYPE_SAW, 0, -1, 222, 2, 1 },{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },};
static unsigned char IMCERROR_ChannelVol[8] = { 100, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCERROR_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCERROR_ChannelStopNote[8] = { true, false, false, false, false, false, false, false };
static TImcSongData imcDataIMCERROR = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2594, /*ENVLISTSIZE*/ 2, /*ENVCOUNTERLISTSIZE*/ 3, /*OSCLISTSIZE*/ 9, /*EFFECTLISTSIZE*/ 0, /*VOL*/ 100,
	IMCERROR_OrderTable, IMCERROR_PatternData, IMCERROR_PatternLookupTable, IMCERROR_EnvList, IMCERROR_EnvCounterList, IMCERROR_OscillatorList, NULL,
	IMCERROR_ChannelVol, IMCERROR_ChannelEnvCounter, IMCERROR_ChannelStopNote };
ZL_Sound sndError = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCERROR);

static const unsigned int IMCCLICK_OrderTable[] = {0x000000001,};
static const unsigned char IMCCLICK_PatternData[] = {0x32, 0x39, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
static const unsigned char IMCCLICK_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCCLICK_EnvList[] = {{ 0, 256, 65, 8, 16, 4, true, 255, },{ 0, 256, 370, 8, 12, 255, true, 255, },};
static TImcSongEnvelopeCounter IMCCLICK_EnvCounterList[] = {{ 0, 0, 256 }, { 1, 0, 256 }, { -1, -1, 256 },};
static const TImcSongOscillator IMCCLICK_OscillatorList[] = {{ 9, 85, IMCSONGOSCTYPE_SQUARE, 0, -1, 126, 1, 2 },{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },};
static unsigned char IMCCLICK_ChannelVol[8] = { 51, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCCLICK_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCCLICK_ChannelStopNote[8] = { false, false, false, false, false, false, false, false };
static TImcSongData imcDataIMCCLICK = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2594, /*ENVLISTSIZE*/ 2, /*ENVCOUNTERLISTSIZE*/ 3, /*OSCLISTSIZE*/ 8, /*EFFECTLISTSIZE*/ 0, /*VOL*/ 100,
	IMCCLICK_OrderTable, IMCCLICK_PatternData, IMCCLICK_PatternLookupTable, IMCCLICK_EnvList, IMCCLICK_EnvCounterList, IMCCLICK_OscillatorList, NULL,
	IMCCLICK_ChannelVol, IMCCLICK_ChannelEnvCounter, IMCCLICK_ChannelStopNote };
ZL_Sound sndClick = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCCLICK);

static const unsigned int IMCCONNECT_OrderTable[] = {0x000000001,};
static const unsigned char IMCCONNECT_PatternData[] = {0x65, 0x64, 0x62, 0x60, 0x65, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
static const unsigned char IMCCONNECT_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCCONNECT_EnvList[] = {{ 0, 256, 65, 8, 16, 255, true, 255, },};
static TImcSongEnvelopeCounter IMCCONNECT_EnvCounterList[] = {{ 0, 0, 256 }, { -1, -1, 256 },};
static const TImcSongOscillator IMCCONNECT_OscillatorList[] = {{ 8, 210, IMCSONGOSCTYPE_SQUARE, 0, -1, 138, 1, 1 },{ 8, 85, IMCSONGOSCTYPE_SQUARE, 0, 0, 50, 1, 1 },{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 6, -1, 100, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 7, -1, 100, 0, 0 },};
static const TImcSongEffect IMCCONNECT_EffectList[] = {{ 126, 0, 5906, 0, IMCSONGEFFECTTYPE_DELAY, 0, 0 },{ 51, 0, 1, 0, IMCSONGEFFECTTYPE_LOWPASS, 1, 0 },};
static unsigned char IMCCONNECT_ChannelVol[8] = { 143, 100, 100, 100, 100, 100, 100, 100 };
static const unsigned char IMCCONNECT_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCCONNECT_ChannelStopNote[8] = { true, false, false, false, false, false, false, false };
static TImcSongData imcDataIMCCONNECT = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 2953, /*ENVLISTSIZE*/ 1, /*ENVCOUNTERLISTSIZE*/ 2, /*OSCLISTSIZE*/ 9, /*EFFECTLISTSIZE*/ 2, /*VOL*/ 100,
	IMCCONNECT_OrderTable, IMCCONNECT_PatternData, IMCCONNECT_PatternLookupTable, IMCCONNECT_EnvList, IMCCONNECT_EnvCounterList, IMCCONNECT_OscillatorList, IMCCONNECT_EffectList,
	IMCCONNECT_ChannelVol, IMCCONNECT_ChannelEnvCounter, IMCCONNECT_ChannelStopNote };
ZL_Sound sndConnect = ZL_SynthImcTrack::LoadAsSample(&imcDataIMCCONNECT);

static const unsigned int IMCBUILD_OrderTable[] = {0x000000001,};
static const unsigned char IMCBUILD_PatternData[] = {0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
static const unsigned char IMCBUILD_PatternLookupTable[] = { 0, 1, 1, 1, 1, 1, 1, 1, };
static const TImcSongEnvelope IMCBUILD_EnvList[] = {{ 0, 256, 435, 24, 255, 255, true, 255, },{ 0, 128, 15, 32, 8, 255, true, 255, },{ 0, 256, 871, 25, 255, 255, true, 255, },{ 0, 148, 16, 30, 10, 255, true, 255, },{ 0, 256, 871, 8, 255, 255, true, 255, },};
static TImcSongEnvelopeCounter IMCBUILD_EnvCounterList[] = {{ -1, -1, 256 }, { 0, 0, 0 }, { 1, 0, 64 }, { 2, 0, 2 },{ 3, 0, 41 }, { 4, 0, 256 },};
static const TImcSongOscillator IMCBUILD_OscillatorList[] = {{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 166, 1, 2 },{ 8, 0, IMCSONGOSCTYPE_SINE, 0, -1, 255, 3, 4 },{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 166, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 255, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 166, 0, 0 },{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 255, 0, 0 },};
static const TImcSongEffect IMCBUILD_EffectList[] = {{ 7366, 204, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 5 },};
static unsigned char IMCBUILD_ChannelVol[8] = { 84, 0, 0, 100, 100, 100, 100, 100 };
static const unsigned char IMCBUILD_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const bool IMCBUILD_ChannelStopNote[8] = { true, true, true, false, false, false, false, false };
TImcSongData imcDataIMCBUILD = {
	/*LEN*/ 0x1, /*ROWLENSAMPLES*/ 16537000, /*ENVLISTSIZE*/ 5, /*ENVCOUNTERLISTSIZE*/ 6, /*OSCLISTSIZE*/ 6, /*EFFECTLISTSIZE*/ 1, /*VOL*/ 255,
	IMCBUILD_OrderTable, IMCBUILD_PatternData, IMCBUILD_PatternLookupTable, IMCBUILD_EnvList, IMCBUILD_EnvCounterList, IMCBUILD_OscillatorList, IMCBUILD_EffectList,
	IMCBUILD_ChannelVol, IMCBUILD_ChannelEnvCounter, IMCBUILD_ChannelStopNote };
ZL_SynthImcTrack sndBuild = ZL_SynthImcTrack(&imcDataIMCBUILD);


static const unsigned int IMCSONG_OrderTable[] = { 0x011000001, 0x011000002, 0x011000003, 0x012000004, 0x021000005, 0x021000006, 0x021000007, 0x012000008, };
static const unsigned char IMCSONG_PatternData[] = {
	0x42, 0x42, 0x42, 0, 0x4B, 0, 0x47, 0, 0x49, 0x47, 0x47, 0, 0x46, 0, 0x46, 0,
	0x42, 0x42, 0x42, 0, 0x49, 0, 0x46, 0, 0x47, 0x46, 0x44, 0, 0x42, 0, 0x42, 0,
	0x42, 0x42, 0x42, 0, 0x47, 0x49, 0x4B, 0, 0x49, 0x47, 0x44, 0, 0x49, 0x4B, 0x50, 0,
	0x4B, 0x49, 0x42, 0, 0x50, 0, 0x4B, 0, 0x49, 0, 0x47, 0, 0, 0, 0, 0,
	0x47, 0, 0, 0x47, 0x4B, 0, 0x47, 0, 0x49, 0, 0, 0x49, 0x49, 0, 0, 0,
	0x49, 0, 0, 0x49, 0x50, 0, 0x49, 0, 0x4B, 0, 0, 0x4B, 0x4B, 0, 0, 0,
	0x4B, 0, 0, 0x4B, 0x52, 0, 0x4B, 0, 0x50, 0, 0, 0x50, 0x50, 0x4B, 0x49, 0x44,
	0x42, 0, 0, 0, 0x46, 0, 0, 0, 0x47, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0x40, 0, 0, 0, 0x40, 0, 0, 0, 0x40, 0, 0, 0, 0x40, 0,
	0, 0, 0x40, 0, 0, 0, 0x40, 0, 0, 0, 0x40, 0, 0x40, 0, 0x40, 0,
	0x40, 0, 0, 0, 0x40, 0, 0, 0, 0x40, 0, 0, 0, 0x40, 0, 0, 0,
	0x40, 0, 0x35, 0, 0x40, 0, 0x35, 0, 0x40, 0, 0x35, 0, 0x40, 0, 0x35, 0,
};
static const unsigned char IMCSONG_PatternLookupTable[] = { 0, 8, 8, 8, 8, 8, 8, 10, };
static const TImcSongEnvelope IMCSONG_EnvList[] = { { 153, 256, 87, 8, 16, 0, true, 255, }, { 0, 256, 182, 8, 16, 255, true, 255, }, { 0, 256, 65, 8, 16, 255, true, 255, }, { 100, 256, 209, 8, 255, 255, true, 255, }, { 0, 256, 523, 1, 23, 255, true, 255, }, { 128, 256, 174, 8, 16, 16, true, 255, }, { 0, 256, 2092, 8, 16, 16, true, 255, }, { 0, 256, 523, 8, 16, 255, true, 255, }, { 0, 256, 379, 8, 16, 255, true, 255, }, { 32, 256, 196, 8, 16, 255, true, 255, }, };
static TImcSongEnvelopeCounter IMCSONG_EnvCounterList[] = { { 0, 0, 256 }, { -1, -1, 256 }, { 1, 0, 256 }, { 2, 0, 256 }, { 3, 0, 256 }, { 4, 6, 158 }, { 5, 6, 256 }, { 6, 6, 256 }, { 7, 6, 256 }, { 8, 7, 256 }, { 9, 7, 256 }, };
static const TImcSongOscillator IMCSONG_OscillatorList[] = { { 6, 253, IMCSONGOSCTYPE_SINE, 0, -1, 255, 1, 1 }, { 7, 253, IMCSONGOSCTYPE_SINE, 0, -1, 204, 1, 1 },
	{ 8, 127, IMCSONGOSCTYPE_SINE, 0, -1, 89, 1, 1 }, { 9, 64, IMCSONGOSCTYPE_SINE, 0, -1, 25, 1, 1 }, { 9, 192, IMCSONGOSCTYPE_SINE, 0, -1, 30, 1, 1 },
	{ 8, 127, IMCSONGOSCTYPE_SINE, 0, -1, 163, 2, 1 }, { 9, 192, IMCSONGOSCTYPE_SINE, 0, -1, 38, 3, 1 }, { 8, 0, IMCSONGOSCTYPE_SINE, 1, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 2, -1, 100, 0, 0 }, { 8, 0, IMCSONGOSCTYPE_SINE, 3, -1, 100, 0, 0 }, { 8, 0, IMCSONGOSCTYPE_SINE, 4, -1, 100, 0, 0 },
	{ 8, 0, IMCSONGOSCTYPE_SINE, 5, -1, 100, 0, 0 }, { 5, 15, IMCSONGOSCTYPE_SINE, 6, -1, 102, 1, 6 }, { 8, 0, IMCSONGOSCTYPE_NOISE, 6, -1, 142, 7, 1 },
	{ 5, 227, IMCSONGOSCTYPE_SINE, 6, -1, 92, 8, 1 }, { 8, 0, IMCSONGOSCTYPE_NOISE, 7, -1, 127, 1, 10 } };
static const TImcSongEffect IMCSONG_EffectList[] = { { 32385, 107, 1, 0, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 4 }, { 50, 0, 1, 6, IMCSONGEFFECTTYPE_LOWPASS, 1, 0 }, { 5461, 1536, 1, 6, IMCSONGEFFECTTYPE_OVERDRIVE, 0, 1 },
	{ 128, 0, 7739, 7, IMCSONGEFFECTTYPE_DELAY, 0, 0 }, { 255, 110, 1, 7, IMCSONGEFFECTTYPE_RESONANCE, 1, 1 }, { 227, 0, 1, 7, IMCSONGEFFECTTYPE_HIGHPASS, 1, 0 } };
static unsigned char IMCSONG_ChannelVol[8] = { 81, 100, 100, 100, 100, 100, 117, 97 };
static const unsigned char IMCSONG_ChannelEnvCounter[8] = { 0, 0, 0, 0, 0, 0, 5, 9 };
static const bool IMCSONG_ChannelStopNote[8] = { false, false, false, false, false, false, true, true };
static TImcSongData imcDataIMCSONG = {
	/*LEN*/ 0x8, /*ROWLENSAMPLES*/ 6615, /*ENVLISTSIZE*/ 10, /*ENVCOUNTERLISTSIZE*/ 11, /*OSCLISTSIZE*/ 16, /*EFFECTLISTSIZE*/ 6, /*VOL*/ 100,
	IMCSONG_OrderTable, IMCSONG_PatternData, IMCSONG_PatternLookupTable, IMCSONG_EnvList, IMCSONG_EnvCounterList, IMCSONG_OscillatorList, IMCSONG_EffectList,
	IMCSONG_ChannelVol, IMCSONG_ChannelEnvCounter, IMCSONG_ChannelStopNote };
ZL_SynthImcTrack sndSong = ZL_SynthImcTrack(&imcDataIMCSONG);
