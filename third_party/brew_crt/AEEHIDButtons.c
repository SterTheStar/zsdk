/*===========================================================================
  FILE:         AEEHIDButtons.c

  SERVICES:     Human Interface Device Helper functions for button access

  DESCRIPTION:  Helpers for mapping button UIDs into standard Zeebo IDs

=============================================================================
        Copyright (c) 2008 Tectoy Digital.
        Author: Tiago Monteiro Fernandes.
        All Rights Reserved.
        Tectoy Digital Confidential and Proprietary.
===========================================================================*/

#include "AEEHIDButtons.h"
#include "AEEHIDDevice_Joystick.h"
#include "AEEStdLib.h"

/*
================================================================================

AEEHIDButton_Uid2Id()

Description:
   Maps an UID into a standard Zeebo ID. When a button is pressed, it has a UID 
   and an ID associated with it. However, the ID might not be the expected, based
   on the standard Zeebo UID-ID mapping. This method converts the UID to this 
   standard mapping ID.

Prototype:
   AEEResult AEEHIDButton_Uid2Id(int uid, 
                                 int* id)

Parameters:
   uid : [in]  : Button UID.
   id  : [out] : Pointer to an int object that will contain the UID mapped into 
                 an ID.

Return Value:

   AEE_SUCCESS : if the mapping was successful.
   EFAILED     : if the UID cannot be mapped into a valid Zeebo ID.

Comments:
   None.

Side Effects:
   None

See Also:
   AEEHIDButton_GetNextButtonEvent

================================================================================
*/
static AEEResult AEEHIDButton_Uid2Id(int uid, 
                                     int* id)
{
    AEEResult returnValue = AEE_SUCCESS;

    switch (uid)
    {
    case AEEUID_HIDJoystick_DPad_Up:
        (*id) =  ZEEBO_BUTTON_DPAD_UP;
        break;
    case AEEUID_HIDJoystick_DPad_Left:
        (*id) =  ZEEBO_BUTTON_DPAD_LEFT;
        break;
    case AEEUID_HIDJoystick_DPad_Down:
        (*id) =  ZEEBO_BUTTON_DPAD_DOWN;
        break;
    case AEEUID_HIDJoystick_DPad_Right:
        (*id) =  ZEEBO_BUTTON_DPAD_RIGHT;
        break;
    case AEEUID_HIDJoystick_Start:
        // BUTTON_START - Not available at Zeebo
        returnValue = EFAILED;
        break;
    case AEEUID_HIDJoystick_Back:
        (*id) =  ZEEBO_BUTTON_HOME;        
        break;
    case AEEUID_HIDJoystick_Left_Thumbstick:
        // BUTTON_LEFT_THUMBSTICK_PRESS - Not available at Zeebo
        returnValue = EFAILED;
        break;
    case AEEUID_HIDJoystick_Right_Thumbstick:
        // BUTTON_RIGHT_THUMBSTICK_PRESS - Not available at Zeebo
        returnValue = EFAILED;
        break;
    case AEEUID_HIDJoystick_Left_Shoulder_Upper:
        (*id) =  ZEEBO_BUTTON_SHOULDER_LEFT;
        break;
    case AEEUID_HIDJoystick_Left_Shoulder_Lower:
        // BUTTON_SHOULDER_LEFT_2 - Not available at Zeebo
        returnValue = EFAILED;
        break;     
    case AEEUID_HIDJoystick_Right_Shoulder_Upper:
        (*id) =  ZEEBO_BUTTON_SHOULDER_RIGHT;
        break;
    case AEEUID_HIDJoystick_Right_Shoulder_Lower:
        // BUTTON_SHOULDER_RIGHT_2 - Not available at Zeebo
        returnValue = EFAILED;
        break;
    case AEEUID_HIDJoystick_Button_1:
        (*id) =  ZEEBO_BUTTON_WEST;
        break;
    case AEEUID_HIDJoystick_Button_2:
        (*id) =  ZEEBO_BUTTON_SOUTH;
        break;
    case AEEUID_HIDJoystick_Button_3:
        (*id) =  ZEEBO_BUTTON_EAST;
        break;
    case AEEUID_HIDJoystick_Button_4:
        (*id) =  ZEEBO_BUTTON_NORTH;
        break;
    default:
        returnValue = EFAILED;
        break;
    }

    return returnValue;
}

/*
================================================================================

AEEHIDButton_GetNextButtonEvent()

Description:
   This function retrieves the next button pressed. It is basicaly a wrapper for 
   IHIDDevice_GetNextButtonEvent. It calls IHIDDevice_GetNextButtonEvent and, if
   successful, try to map the resulting button UID into a valid Zeebo ID. The 
   valid Zeebo IDs are listed on AEEHIDButtons.h.
   Errors during this mapping or during IHIDDevice_GetNextButtonEvent will result
   in this method returning an error.

Prototype:
   AEEResult AEEHIDButton_GetNextButtonEvent(IHIDDevice* piHIDDevice, 
                                             AEEHIDButtonInfo* pnButtonInfo, 
                                             uint32* pdwTimestamp, 
                                             boolean* pbDroppedEvents)

Parameters:
   piHIDDevice           : [in]  : Pointer to an IHIDDevice object.
   pnButtonInfo          : [out] : Pointer to an AEEHIDButtonInfo structure that 
                                   will contain the information of the next button 
                                   on the buttons pressed queue.
   pdwTimestamp          : [out] : Pointer to an uint32 object that will be used
                                   on the IHIDDevice_GetNextButtonEvent call.
   pbDroppedEvents       : [out] : Pointer to a boolean object that will be used
                                   on the IHIDDevice_GetNextButtonEvent call.

Return Value:

   AEE_SUCCESS      : if IHIDDevice_GetNextButtonEvent returned TRUE and the button
                      pressed can be mapped into a valid Zeebo button.

   Another appropriate error code if operation was not successful.

Comments:
   None.

Side Effects:
   None

See Also:
   IHIDDevice_GetNextButtonEvent, AEEHIDButton_Uid2Id

================================================================================
*/
AEEResult AEEHIDButton_GetNextButtonEvent(IHIDDevice* piHIDDevice, 
                                          AEEHIDButtonInfo* pnButtonInfo, 
                                          uint32* pdwTimestamp, 
                                          boolean* pbDroppedEvents)
 {
    AEEResult returnValue;

    returnValue = IHIDDevice_GetNextButtonEvent(piHIDDevice, pnButtonInfo, pdwTimestamp, pbDroppedEvents);

    if (returnValue == AEE_SUCCESS)
    {
        returnValue = AEEHIDButton_Uid2Id(pnButtonInfo->nButtonUID, &pnButtonInfo->nButtonID);
    }

    return returnValue;
}
