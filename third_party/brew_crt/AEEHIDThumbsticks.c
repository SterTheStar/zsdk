/*===========================================================================
  FILE:         AEEHIDThumbsticks.c

  SERVICES:     Human Interface Device Helper functions for thumbstick access

  DESCRIPTION:  Helpers for interpreting the HID axis reports.

=============================================================================
        Copyright (c) 2008 QUALCOMM Incorporated.
               All Rights Reserved.
        Qualcomm Confidential and Proprietary
===========================================================================*/

#include "AEEHIDThumbsticks.h"
#include "AEEHIDDevice_Joystick.h"
#include "AEEStdLib.h"

struct AEEHIDThumbstickMapping
{
   AEEHIDPositionInfo piMin;
   AEEHIDPositionInfo piMax;
   AEEHIDPositionInfo piInfo;
   int nLXOffset;
   int nLYOffset;
   int nRXOffset;
   int nRYOffset;
};


/*
================================================================================

AEEHIDThumbstick_InitializeMapping()

Description:
   Used to construct a mapping table from HID axes to thumbstick axes.
   The UIDs that are retrieved from IHIDDevice_GetAxesInfo will be used to build
   this table. If an axis is not found in the info retrieved from this function
   the first unassigned axis will be added to the mapping table. If the device has
   less than four axes this function will return an error.

Prototype:
   AEEResult AEEHIDThumbstick_InitializeMapping(IHIDDevice *piHIDDevice, 
                                                AEEHIDThumbstickMapping **ppHIDThumbstickMapping)

Parameters:
   piHIDDevice  : [in]  : Pointer to an IHIDDevice object
   ppHIDThumbstickMapping : [out] : Pointer to a pointer to an AEEHIDThumbstickMapping object that
                                    will represent the mapping data. 
 
Return Value:

   AEE_SUCCESS      : Mapping info was created.
   
   Another appropriate error code if operation was not successful

Comments:
   None.

Side Effects:
   None

See Also:
   AEEHIDThumbstick_GetPositionState, AEEHIDThumbstick_DestroyMapping
   
================================================================================
*/
AEEResult AEEHIDThumbstick_InitializeMapping(IHIDDevice *piHIDDevice, 
                                             AEEHIDThumbstickMapping **ppHIDThumbstickMapping)
{
   AEEResult nErr;
   int nUseUnassigned = 0;

   if(NULL == ppHIDThumbstickMapping)
   {
      return AEE_EBADPARM;
   }

   *ppHIDThumbstickMapping = (AEEHIDThumbstickMapping *)MALLOC(sizeof(AEEHIDThumbstickMapping));
   if(*ppHIDThumbstickMapping == NULL)
   {
      return AEE_ENOMEMORY;
   }
   
   nErr = IHIDDevice_GetMinPositionInfo(piHIDDevice, &((*ppHIDThumbstickMapping)->piMin));
   if(AEE_SUCCESS != nErr)
   {
      goto Exit;
   }

   nErr = IHIDDevice_GetMaxPositionInfo(piHIDDevice, &((*ppHIDThumbstickMapping)->piMax));
   if(AEE_SUCCESS != nErr)
   {
      goto Exit;
   }

   nErr = IHIDDevice_GetAxesInfo(piHIDDevice, &((*ppHIDThumbstickMapping)->piInfo));
   if(AEE_SUCCESS != nErr)
   {
      goto Exit;
   }

   //Find Left X
   do
   {
      if((*ppHIDThumbstickMapping)->piInfo.nX == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nY == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nZ == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRx == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRy == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRz == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVX == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVY == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVZ == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRx == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRy == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRz == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAX == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAY == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAZ == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARx == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARy == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARz == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFX == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFY == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFZ == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRx == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRy == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRz == AEEUID_HIDJoystick_LeftThumb_X)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }

      //Not found find first 0
      nUseUnassigned++;
      if((*ppHIDThumbstickMapping)->piInfo.nX == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nY == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nZ == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRx == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRy == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRz == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVX == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVY == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVZ == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRx == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRy == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRz == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAX == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAY == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAZ == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARx == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARy == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARz == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFX == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFY == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFZ == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRx == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRy == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRz == 0)
      {
         (*ppHIDThumbstickMapping)->nLXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }

      nErr = AEE_EFAILED;
      goto Exit;
   } while(0);

   //Find Left Y
   do
   {
      int nFound = 0;
      if((*ppHIDThumbstickMapping)->piInfo.nX == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nY == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nZ == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRx == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRy == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRz == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVX == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVY == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVZ == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRx == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRy == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRz == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAX == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAY == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAZ == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARx == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARy == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARz == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFX == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFY == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFZ == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRx == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRy == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRz == AEEUID_HIDJoystick_LeftThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }

      //Not found find first 0
      nUseUnassigned++;
      if((*ppHIDThumbstickMapping)->piInfo.nX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nLYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }

      nErr = AEE_EFAILED;
      goto Exit;
   } while(0);

   //Find Right X
   do
   {
      int nFound = 0;
      if((*ppHIDThumbstickMapping)->piInfo.nX == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nY == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nZ == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRx == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRy == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRz == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVX == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVY == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVZ == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRx == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRy == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRz == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAX == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAY == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAZ == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARx == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARy == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARz == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFX == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFY == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFZ == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRx == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRy == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRz == AEEUID_HIDJoystick_RightThumb_X)
      {
         (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }

      //Not found find first 0
      nUseUnassigned++;
      if((*ppHIDThumbstickMapping)->piInfo.nX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRXOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }

      nErr = AEE_EFAILED;
      goto Exit;
   } while(0);

   //Find Right Y
   do
   {
      int nFound = 0;
      if((*ppHIDThumbstickMapping)->piInfo.nX == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nY == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nZ == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRx == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRy == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nRz == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVX == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVY == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVZ == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRx == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRy == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nVRz == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAX == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAY == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nAZ == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARx == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARy == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nARz == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFX == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFY == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFZ == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRx == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRy == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }
      else if((*ppHIDThumbstickMapping)->piInfo.nFRz == AEEUID_HIDJoystick_RightThumb_Y)
      {
         (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
         break;
      }

      //Not found find first 0
      nUseUnassigned++;
      if((*ppHIDThumbstickMapping)->piInfo.nX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVZ)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nVRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nVRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nAZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nAY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nARz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nARz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFX == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFX)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFY == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFZ == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFY)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRx == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRx)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRy == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRy)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }
      if((*ppHIDThumbstickMapping)->piInfo.nFRz == 0)
      {
         nFound++;
         if(nUseUnassigned == nFound)
         {
            (*ppHIDThumbstickMapping)->nRYOffset = ((byte *)&((*ppHIDThumbstickMapping)->piInfo.nFRz)) - ((byte *)&((*ppHIDThumbstickMapping)->piInfo));
            break;
         }
      }

      nErr = AEE_EFAILED;
      goto Exit;
   } while(0);

Exit:
   if(AEE_SUCCESS != nErr)
   {
      FREE(*ppHIDThumbstickMapping);
      *ppHIDThumbstickMapping = NULL;
   }
   return nErr;
}

/*
================================================================================

AEEHIDThumbstick_GetPositionState()

Description:
   This function retrieves the current device info. The mapping info is the data that
   was created from AEEHIDThumbstick_InitializeMapping and the pHIDThumbstickValues
   struct will contain the normalized axes values. These values will always go from -32768 to 32767.

Prototype:
   AEEResult AEEHIDThumbstick_GetPositionState(IHIDDevice *piHIDDevice,
                                               AEEHIDThumbstickMapping *pHIDThumbstickMapping,
                                               AEEHIDThumbstickValues *pHIDThumbstickValues);

Parameters:
   piHIDDevice           : [in]  : Pointer to an IHIDDevice object
   pHIDThumbstickMapping : [in]  : Pointer to an AEEHIDThumbstickMapping object that
                                    will represent the mapping data. If this value is NULL
                                    the mapping info will be constructed each time this function
                                    is called.
   pHIDThumbstickValues  : [out] : Pointer to an AEEHIDThumbstickValues structure that will
                                   contain the axes values.

Return Value:

   AEE_SUCCESS      : if the axes values were retrieved

   Another appropriate error code if operation was not successful

Comments:
   None.

Side Effects:
   None

See Also:
   AEEHIDThumbstick_InitializeMapping


================================================================================
*/
AEEResult AEEHIDThumbstick_GetPositionState(IHIDDevice *piHIDDevice,
                                            AEEHIDThumbstickMapping *pHIDThumbstickMapping,
                                            AEEHIDThumbstickValues *pHIDThumbstickValues)
{
   boolean bNeedFree = FALSE;
   AEEResult nErr;
   int nLX, nLY, nRX, nRY;
   int nMinLX, nMinLY, nMinRX, nMinRY;
   int nMaxLX, nMaxLY, nMaxRX, nMaxRY;
   AEEHIDPositionInfo pi;

   if(NULL == pHIDThumbstickValues)
   {
      return AEE_EBADPARM;
   }

   nErr = IHIDDevice_GetPositionState(piHIDDevice, &pi);
   if(AEE_SUCCESS != nErr)
   {
      return nErr;
   }

   if(NULL == pHIDThumbstickMapping)
   {
      nErr = AEEHIDThumbstick_InitializeMapping(piHIDDevice, &pHIDThumbstickMapping);
      bNeedFree = TRUE;
      if(nErr != AEE_SUCCESS)
      {
         return nErr;
      }
   }

   nLX = *(int *) (((byte *)(void *)&pi) + pHIDThumbstickMapping->nLXOffset);
   nLY = *(int *) (((byte *)(void *)&pi) + pHIDThumbstickMapping->nLYOffset);
   nRX = *(int *) (((byte *)(void *)&pi) + pHIDThumbstickMapping->nRXOffset);
   nRY = *(int *) (((byte *)(void *)&pi) + pHIDThumbstickMapping->nRYOffset);

   nMinLX = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMin)) + pHIDThumbstickMapping->nLXOffset);
   nMinLY = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMin)) + pHIDThumbstickMapping->nLYOffset);
   nMinRX = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMin)) + pHIDThumbstickMapping->nRXOffset);
   nMinRY = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMin)) + pHIDThumbstickMapping->nRYOffset);

   nMaxLX = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMax)) + pHIDThumbstickMapping->nLXOffset);
   nMaxLY = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMax)) + pHIDThumbstickMapping->nLYOffset);
   nMaxRX = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMax)) + pHIDThumbstickMapping->nRXOffset);
   nMaxRY = *(int *) (((byte *)(void *)&(pHIDThumbstickMapping->piMax)) + pHIDThumbstickMapping->nRYOffset);

   //Normalize Value
   nLX = HID_THUMBSTICK_MIN + ((nLX - nMinLX) * (HID_THUMBSTICK_MAX - HID_THUMBSTICK_MIN))/(nMaxLX - nMinLX); //TODO check division by 0
   nLY = HID_THUMBSTICK_MIN + ((nLY - nMinLY) * (HID_THUMBSTICK_MAX - HID_THUMBSTICK_MIN))/(nMaxLY - nMinLY); //TODO check division by 0
   nRX = HID_THUMBSTICK_MIN + ((nRX - nMinRX) * (HID_THUMBSTICK_MAX - HID_THUMBSTICK_MIN))/(nMaxRX - nMinRX); //TODO check division by 0
   nRY = HID_THUMBSTICK_MIN + ((nRY - nMinRY) * (HID_THUMBSTICK_MAX - HID_THUMBSTICK_MIN))/(nMaxRY - nMinRY); //TODO check division by 0

   pHIDThumbstickValues->sLeftX = (int16)nLX;
   pHIDThumbstickValues->sLeftY = (int16)nLY;
   pHIDThumbstickValues->sRightX = (int16)nRX;
   pHIDThumbstickValues->sRightY = (int16)nRY;



   if(bNeedFree)
   {
      AEEHIDThumbstick_DestroyMapping(piHIDDevice, pHIDThumbstickMapping);
   }
   return AEE_SUCCESS;

}

/*
================================================================================

AEEHIDThumbstick_DestroyMapping()

Description:
   Used to destroy the mapping data. Should be invoked when the device is disconnected.

Prototype:
   AEEResult AEEHIDThumbstick_DestroyMapping(IHIDDevice *piHIDDevice, 
                                             AEEHIDThumbstickMapping *pHIDThumbstickMapping)

Parameters:
   piHIDDevice           : [in]  : Pointer to an IHIDDevice object
   pHIDThumbstickMapping : [in]  : Pointer to an AEEHIDThumbstickMapping object that
                                    will be destroyed. 

Return Value:

   AEE_SUCCESS      : if the mapping data was destroyed.

   Another appropriate error code if operation was not successful

Comments:
   None.

Side Effects:
   None

See Also:
   AEEHIDThumbstick_InitializeMapping

================================================================================
*/
AEEResult AEEHIDThumbstick_DestroyMapping(IHIDDevice *piHIDDevice, 
                                          AEEHIDThumbstickMapping *pHIDThumbstickMapping)
{
   if(NULL == pHIDThumbstickMapping)
   {
      return AEE_EBADPARM;
   }
   else
   {
      FREE(pHIDThumbstickMapping);
      return AEE_SUCCESS;
   }
}
