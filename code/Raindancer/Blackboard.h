/*
Robotic Lawn Mower
Copyright (c) 2017 by Kai WÃ¼rtz

Private-use only! (you need to ask for a commercial-use)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Private-use only! (you need to ask for a commercial-use)
*/

#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "NodeStack.h"

#include "hardware.h"
#include "helpers.h"
#include "batterySensor.h"
#include "motor.h"
#include "perimeter.h"
#include "errorhandler.h"
#include "rangeSensor.h"
#include "bumperSensor.h"
#include "mowmotorSensor.h"
#include "chargeSystem.h"
#include "EEPROM.h"




//Achtung, bei Aenderung auch enuDriveDirectionString[] aendern!!!
enum enuDriveDirection {
    DD_FORWARD = 0,
    DD_FORWARD_INSIDE = 1, //Darf eigentlich nicht vorkommen wenn ich prÃ¼fe, dass Spule hinten rausfÃ¤hrt beim zurÃ¼ckfahren
    DD_OVERRUN = 2,
    DD_REVERSE_ESC_OBST  = 3, 
    DD_REVERSE_INSIDE = 4,
    DD_ROTATECW = 5,
    DD_ROTATECC = 6,
	DD_LINE_FOLLOW = 7,
    DD_SPIRAL_CW = 8,
	DD_FEOROTATECC = 9,
	DD_FEOROTATECW = 10,
	DD_FREEBUMPER = 11, // Only used in History
	DD_NONE = 12 //Only used in History to init the drivedirection
	//DD_ROTATE90CW = 11,
	//DD_ROTATE90CC = 12
};

extern const char* enuDriveDirectionString[];  //in Blackboard.cpp definiert


enum enuFlagCoilsOutside  {CO_NONE, CO_RIGHT, CO_LEFT, CO_BOTH };
extern const char* enuFlagCoilsOutsideString[]; //in Blackboard.cpp definiert

enum enuFlagPeriStateChanged  {PSC_NONE, PSC_IO, PSC_OI};

enum enuFlagForceRotateDirection  {FRD_NONE, FRD_CC, FRD_CW};
extern const char* enuFlagForceRotateDirectionString[]; //in Blackboard.cpp definiert

//Achtung, bei Aenderung auch enuFlagEscabeObstacleConFlagString[] aendern!!!
enum enuFlagEscabeObstacleConFlag  {FEO_NONE, FEO_ROTCC, FEO_ROTCW, FEO_BACKINSIDE, FEO_ROT}; // Condition which algortihm will be used by mselEscabeObstacle
extern const char* enuFlagEscabeObstacleConFlagString[]; //in Blackboard.cpp definiert

enum enuBehaviour  {BH_NONE, BH_MOW, BH_PERITRACK, BH_CHARGING, BH_GOTOAREA, BH_FINDPERIMETER, BH_RESTOREHISTORY};


//Structure for history and current rotation data
typedef struct {
 	enuDriveDirection driveDirection;	// [0]  Enthaelt Fahrtrichtung
	float  distanceDriven;				// [0] Enthaelt die gerade gefahrene distanz/winkeldistanz
	float rotAngleSoll;					// [0] Sollwinkel der aktuellen Drehung
	float rotAngleIst;					// [0] Tatsächlich gedrehter Winkel
	enuFlagForceRotateDirection flagForceRotDirection; // welche Richtung wurde gedreht
	enuFlagCoilsOutside   coilFirstOutside; // [0] which coil was first outside jet
	bool restored; // wurde die node restored
	unsigned long timeAdded; // zeit, wann node erstellt wurde in ms

} THistory;
#define HISTROY_BUFSIZE 15

class Node;


// We will use a Blackboard object. This object will be created by the user and will be propagated to the nodes during the Blackboard through the tick function.
class Blackboard
{
public:
    Blackboard(TMotorInterface &_motor, TPerimeterThread &_perimeterSenoren, TMowMotorSensor& _mowMotorSensor, TrangeSensor &_rangeSensor,
               TbumperSensor &_bumperSensor,  TbatterieSensor& _batterieSensor, CRotaryEncoder &_encoderL, CRotaryEncoder &_encoderR, TchargeSystem &_chargeSystem, TEEPROM &_eeprom):
        motor(_motor),
        perimeterSensoren(_perimeterSenoren),
        rangeSensor(_rangeSensor),
        bumperSensor(_bumperSensor),
        mowMotorSensor(_mowMotorSensor),
        batterieSensor(_batterieSensor),
        encoderL(_encoderL),
        encoderR(_encoderR),
        chargeSystem(_chargeSystem),
		eeprom(_eeprom)
    {  
		flagEnableMowing = false;
		flagEnablePerimetertracking = false;
		flagEnableCharging = false;
		flagEnableGotoAreaX = false;
		flagEnableFindPerimeter = false;
		flagEnableRestoreHistory = false;
	}

	THistory history[HISTROY_BUFSIZE];
	int8_t numberToRestoreHist;

	long randAngle;

	static const int CRUISE_SPEED_HIGH = 85;//92,90
    static const int CRUISE_SPEED_MEDIUM = 70;
    static const int CRUISE_SPEED_LOW = 50;
    static const int CRUISE_SPEED_OBSTACLE = 40;  // Lowest Speed to drive
	static const int CRUISE_ROTATE_HIGH = 40;
	static const int CRUISE_ROTATE_LOW = 25;
	static const int LINEFOLLOW_SPEED_HIGH = 50;
	static const int LINEFOLLOW_SPEED_LOW = 50;
    static const int SHORTWAYCOUNT  = 3; //3


	enuFlagCoilsOutside   flagCoilFirstOutsideLatched;
    enuFlagCoilsOutside   flagCoilFirstOutside;
    //enuFlagCoilsOutside   flagCoilOutsideAfterOverrun;
    enuFlagPeriStateChanged  flagPerimeterStateChanged;
    enuFlagForceRotateDirection flagForceRotateDirection;
    enuFlagEscabeObstacleConFlag flagEscabeObstacleConFlag;


    bool flagBumperInsidePerActivated ;
    bool flagBumperOutsidePerActivated;
    bool flagCruiseSpiral;
	bool flagForceAngle;
	bool flagDeactivateRotInside;
    //bool flagPerimeterActivated;
	bool flagRotateAtPer;
	bool flagDriveCurve;
    
    int flagForceSmallRotAngle; // Anzahl, wie haeufig kleiner Winkel gedreht werden soll 


    bool flagEnableMowing;
    bool flagEnablePerimetertracking;
    bool flagEnableCharging;
    bool flagEnableGotoAreaX;
    bool flagEnableFindPerimeter;
	bool flagEnableRestoreHistory;
    
    bool flagGotoAreaXFirstCall;

    bool flagGoHome;

    int cruiseSpeed;
    unsigned long timeCruiseSpeedSet;

	int32_t numberOfRotations;
	unsigned long timeInMowBehaviour;
	
    enuDriveDirection driveDirection;

    int arcRotateXArc;

    long areaTargetDistanceInMeter;

    unsigned long lastTimeSpiralStarted;
   
	bool flagBHTShowLastNode; //shows last nodes and conditions on the terminal

	bool flagShowRotateX;
	bool flagShowHistory;

	float coilsOutsideAngle;
	bool flagEnableSecondReverse;

    TMotorInterface& motor;
    TPerimeterThread& perimeterSensoren;
    TrangeSensor& rangeSensor;
    TbumperSensor& bumperSensor;
    TMowMotorSensor& mowMotorSensor;
    TbatterieSensor& batterieSensor;
    CRotaryEncoder& encoderL;
    CRotaryEncoder& encoderR;
    TchargeSystem& chargeSystem;
	TEEPROM& eeprom;

    // Wird von BehaviourTree funktionen verwendet und muss immer vorhanden sein.
    Node* lastNodeLastRun;
    Node* lastNodeCurrentRun;
    NodeStack runningNodes;
    //

	void resetBB();
    void setBehaviour(enuBehaviour b );
	//void addHistoryEntry(THistory &h);
	void addHistoryEntry(enuDriveDirection _driveDirection, float  _distanceDriven, float _rotAngleSoll, float _rotAngleIst, enuFlagForceRotateDirection _flagForceRotDirection, enuFlagCoilsOutside   _coilFirstOutside);
	void markLastHistoryEntryAsRestored();
	bool histGetTwoLastForwardDistances(float& a, float& b);


};


#endif
