#ifndef tkmFunctionalVolume_h
#define tkmFunctionalVolume_h

#include "xDebug.h"
#include "xTypes.h"
#include "tkmVoxel.h"
#include "tkmFunctionalDataAccess.h"
#include "xList.h"
#include "tkmedit.h"
#include "tcl.h"

typedef enum {
  FunV_tErr_NoError = 0,
  FunV_tErr_AllocationFailed,
  FunV_tErr_DeletionFailed,
  FunV_tErr_ErrorParsingPathAndStem,
  FunV_tErr_ErrorLoadingVolume,
  FunV_tErr_ErrorAccessingInternalVolume,
  FunV_tErr_ErrorAccessingSelectionList,
  FunV_tErr_ErrorConvertingSecondToTimePoint,
  FunV_tErr_OverlayNotLoaded,
  FunV_tErr_TimeCourseNotLoaded,
  FunV_tErr_GraphWindowAlreadyInited,
  FunV_tErr_GraphWindowNotInited,
  FunV_tErr_ErrorFindingScriptTclFile,
  FunV_tErr_ErrorParsingScriptTclFile,
  FunV_tErr_InvalidPointer,
  FunV_tErr_InvalidSignature,
  FunV_tErr_InvalidTimePoint,
  FunV_tErr_InvalidCondition,
  FunV_tErr_InvalidAnatomicalVoxel,
  FunV_tErr_InvalidThreshold,
  FunV_tErr_InvalidDisplayFlag,
  FunV_tErr_WrongNumberArgs,
  FunV_tErr_InvalidErrorCode,
  FunV_tErr_knNumErrorCodes
} FunV_tErr;

typedef enum {
  FunV_tDisplayFlag_None = -1,
  FunV_tDisplayFlag_Ol_TruncateOverlay = 0,
  FunV_tDisplayFlag_Ol_ReversePhase,
  FunV_tDisplayFlag_TC_GraphWindowOpen,
  FunV_tDisplayFlag_Ol_OffsetValues,
  FunV_tDisplayFlag_TC_OffsetValues,
  FunV_knNumDisplayFlags
} FunV_tDisplayFlag;

#define FunV_knFirstOverlayDisplayFlag    FunV_tDisplayFlag_Ol_TruncateOverlay
#define FunV_knLastOverlayDisplayFlag     FunV_tDisplayFlag_Ol_ReversePhase
#define FunV_knFirstTimeCourseDisplayFlag FunV_tDisplayFlag_TC_GraphWindowOpen
#define FunV_knLastTimeCourseDisplayFlag  FunV_tDisplayFlag_TC_GraphWindowOpen

typedef enum {
  FunV_tTclCommand_Ol_DoConfigDlog = 0,
  FunV_tTclCommand_Ol_UpdateNumTimePoints,
  FunV_tTclCommand_Ol_UpdateNumConditions,
  FunV_tTclCommand_Ol_UpdateDisplayFlag,
  FunV_tTclCommand_Ol_UpdateDataName,
  FunV_tTclCommand_Ol_UpdateTimePoint,
  FunV_tTclCommand_Ol_UpdateCondition,
  FunV_tTclCommand_Ol_UpdateThreshold,
  FunV_tTclCommand_Ol_ShowOffsetOptions,
  FunV_tTclCommand_TC_DoConfigDlog,
  FunV_tTclCommand_TC_BeginDrawingGraph,
  FunV_tTclCommand_TC_EndDrawingGraph,
  FunV_tTclCommand_TC_DrawGraph,
  FunV_tTclCommand_TC_ClearGraph,
  FunV_tTclCommand_TC_UpdateNumConditions,
  FunV_tTclCommand_TC_UpdateNumPreStimPoints,
  FunV_tTclCommand_TC_UpdateTimeResolution,
  FunV_tTclCommand_TC_UpdateDisplayFlag,
  FunV_tTclCommand_TC_UpdateDataName,
  FunV_tTclCommand_TC_UpdateLocationName,
  FunV_tTclCommand_TC_UpdateGraphData,
  FunV_tTclCommand_TC_UpdateErrorData,
  FunV_tTclCommand_TC_ShowOffsetOptions,
  FunV_knNumTclCommands
} FunV_tTclCommand;

typedef float FunV_tFunctionalValue;

/* main class structure */
struct tkmFunctionalVolume {

  tSignature mSignature;

  /* our volumes. VolumeRef is actually tkmFunctionalDataAccess */
  VolumeRef  mpOverlayVolume;
  VolumeRef  mpTimeCourseVolume;
  VolumeRef  mpOverlayOffsetVolume;
  VolumeRef  mpTimeCourseOffsetVolume;

  /* current drawing state, what section is being displayed */
  xListRef               mpSelectedVoxels;
  int                    mnTimePoint;
  int                    mnCondition;
  FunV_tFunctionalValue  mThresholdMin;
  FunV_tFunctionalValue  mThresholdMid;
  FunV_tFunctionalValue  mThresholdSlope;
  tBoolean               mabDisplayFlags[FunV_knNumDisplayFlags];
  tBoolean               mbGraphInited;
  
  /* functions to access the outside world */
  void       (*mpOverlayChangedFunction)(void);
  void       (*mpSendTkmeditTclCmdFunction)(tkm_tTclCommand,char*);
  void       (*mpSendTclCommandFunction)(char*);
};
typedef struct tkmFunctionalVolume tkmFunctionalVolume;
typedef tkmFunctionalVolume *tkmFunctionalVolumeRef;

#define FunV_kSignature 0x00666000

/* allocator an destructor */
FunV_tErr FunV_New    ( tkmFunctionalVolumeRef* oppVolume,
      void(*ipOverlayChangedFunction)(void),
      void(*ipSendTkmeditCmdFunction)(tkm_tTclCommand,char*),
      void(*ipSendTclCommandFunction)(char*) );
FunV_tErr FunV_Delete ( tkmFunctionalVolumeRef* ioppVolume );

/* loads the data volumes */
FunV_tErr FunV_LoadOverlay    ( tkmFunctionalVolumeRef this,
        char*                  isPathAndStem );
FunV_tErr FunV_LoadTimeCourse ( tkmFunctionalVolumeRef this,
        char*                  isPathAndStem );

/* this loads a specific volume. will delete one if it already exists. */
FunV_tErr FunV_LoadFunctionalVolume_ ( tkmFunctionalVolumeRef this,
               VolumeRef*             ioppVolume,
               char*                  isPath,
               char*                  isStem,
               char*                  isHeaderStem,
               char*                  isRegPath );

/* get status of loadedness */
FunV_tErr FunV_IsOverlayPresent    ( tkmFunctionalVolumeRef this,
             tBoolean*              obIsLoaded );
FunV_tErr FunV_IsTimeCoursePresent ( tkmFunctionalVolumeRef this,
             tBoolean*              obIsLoaded );


/* settors. these check values and if valid, sets internal vars. generates
   proper update msgs for tcl */
FunV_tErr FunV_SetTimeResolution   ( tkmFunctionalVolumeRef this,
             int                    inTimeResolution );
FunV_tErr FunV_SetNumPreStimPoints ( tkmFunctionalVolumeRef this,
             int                    inNumPoints );
FunV_tErr FunV_SetTimeSecond       ( tkmFunctionalVolumeRef this,
             int                    inSecond );
FunV_tErr FunV_SetTimePoint        ( tkmFunctionalVolumeRef this,
             int                    inTimePoint );
FunV_tErr FunV_SetCondition        ( tkmFunctionalVolumeRef this,
             int                    inCondition );
FunV_tErr FunV_SetThreshold        ( tkmFunctionalVolumeRef this,
             FunV_tFunctionalValue  iMin,  
             FunV_tFunctionalValue  iMid,     
             FunV_tFunctionalValue  iSlope ); 
FunV_tErr FunV_SetDisplayFlag      ( tkmFunctionalVolumeRef this,
             FunV_tDisplayFlag      iFlag,
             tBoolean               iNewValue );

/* allows functional volume to respond to a click. */
FunV_tErr FunV_AnatomicalVoxelClicked ( tkmFunctionalVolumeRef this,
                      VoxelRef          ipAnatomicalVoxel );


/* overlay access */

/* basic accessors to values, based on current plane position if
   applicable */
FunV_tErr FunV_GetValueAtAnatomicalVoxel ( tkmFunctionalVolumeRef this,
             VoxelRef               ipVoxel,
             FunV_tFunctionalValue* opValue );
FunV_tErr FunV_GetValueAtRASVoxel        ( tkmFunctionalVolumeRef this,
             VoxelRef               ipVoxel,
             FunV_tFunctionalValue* opValue );

/* calculate the rgb values for a color */
FunV_tErr FunV_GetColorForValue ( tkmFunctionalVolumeRef this,
          FunV_tFunctionalValue  iValue,
          float                  ifBaseValue,
          float*                 ofRed,
          float*                 ofGreen,
          float*                 ofBlue );


/* time course graph access */

/* reads the graph tcl script and sets up the window. also sets up initial
values for functional state. */
FunV_tErr FunV_InitGraphWindow ( tkmFunctionalVolumeRef this,
         Tcl_Interp*            pInterp );

/* for displaying voxels in graph */
FunV_tErr FunV_BeginSelectionRange      ( tkmFunctionalVolumeRef this );
FunV_tErr FunV_AddAnatomicalVoxelToSelectionRange 
                                        ( tkmFunctionalVolumeRef this,
                     VoxelRef               ipVoxel );
FunV_tErr FunV_EndSelectionRange        ( tkmFunctionalVolumeRef this );

/* grabs values for the current selected voxels and shoots them towards
   the graph to be drawn onto the screen. */
FunV_tErr FunV_DrawGraph           ( tkmFunctionalVolumeRef this );
FunV_tErr FunV_SendGraphData_      ( tkmFunctionalVolumeRef this,
             int                    inCondition,
             int                    inNumValues,
             float*                 iafValues );
FunV_tErr FunV_SendGraphErrorBars_ ( tkmFunctionalVolumeRef this,
             int                    inCondition,
             int                    inNumValues,
             float*                 iafBars );

/* tcl commands */
int FunV_TclOlLoadData            ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclOlSetTimePoint        ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclOlSetCondition        ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclOlSetDisplayFlag      ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclOlSetThreshold        ( ClientData inClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclTCLoadData            ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclTCSetNumPreStimPoints ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclTCSetTimeResolution   ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );
int FunV_TclTCSetDisplayFlag      ( ClientData iClientData, 
            Tcl_Interp *ipInterp, 
            int argc, char *argv[] );

/* misc */
FunV_tErr FunV_SendViewStateToTcl  ( tkmFunctionalVolumeRef this );
FunV_tErr FunV_RegisterTclCommands ( tkmFunctionalVolumeRef this,
             Tcl_Interp*            pInterp );
FunV_tErr FunV_GetThresholdMax     ( tkmFunctionalVolumeRef this,
             FunV_tFunctionalValue* oValue );
FunV_tErr FunV_GetThreshold        ( tkmFunctionalVolumeRef this,
             FunV_tFunctionalValue* oMin,  
             FunV_tFunctionalValue* oMid,     
             FunV_tFunctionalValue* oSlope ); 

/* these check to see if we have valid callback functions and if so,
   calls them. */
FunV_tErr FunV_OverlayChanged_        ( tkmFunctionalVolumeRef this );
FunV_tErr FunV_SendTkmeditTclCommand_ ( tkmFunctionalVolumeRef this,
          tkm_tTclCommand        iCommand,
          char*                  isArguments );
FunV_tErr FunV_SendTclCommand_        ( tkmFunctionalVolumeRef this,
          FunV_tTclCommand       iCommand,
          char*                  isArguments );

/* error checking etc */
FunV_tErr FunV_Verify ( tkmFunctionalVolumeRef this );
void FunV_Signal ( char* isFuncName, int inLineNum, FunV_tErr ieCode );
char * FunV_GetErrorString ( FunV_tErr inErr );


#endif
