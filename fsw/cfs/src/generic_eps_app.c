/*******************************************************************************
** File: generic_eps_app.c
**
** Purpose:
**   This file contains the source code for the GENERIC_EPS application.
**
*******************************************************************************/

/*
** Include Files
*/
#include "generic_eps_app.h"
#include "generic_eps_device.h"

/*
** Global Data
*/
GENERIC_EPS_AppData_t GENERIC_EPS_AppData;

/*
** Application entry point and main process loop
*/
void EPS_AppMain(void)
{
    int32 status = OS_SUCCESS;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(GENERIC_EPS_PERF_ID);

    /* 
    ** Perform application initialization
    */
    status = GENERIC_EPS_AppInit();
    if (status != CFE_SUCCESS)
    {
        GENERIC_EPS_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Main loop
    */
    while (CFE_ES_RunLoop(&GENERIC_EPS_AppData.RunStatus) == true)
    {
        /*
        ** Performance log exit stamp
        */
        CFE_ES_PerfLogExit(GENERIC_EPS_PERF_ID);

        /* 
        ** Pend on the arrival of the next Software Bus message
        ** Note that this is the standard, but timeouts are available
        */
        status = CFE_SB_ReceiveBuffer((CFE_SB_Buffer_t **)&GENERIC_EPS_AppData.MsgPtr,  GENERIC_EPS_AppData.CmdPipe,  CFE_SB_PEND_FOREVER);
        
        /* 
        ** Begin performance metrics on anything after this line. This will help to determine
        ** where we are spending most of the time during this app execution.
        */
        CFE_ES_PerfLogEntry(GENERIC_EPS_PERF_ID);

        /*
        ** If the CFE_SB_ReceiveBuffer was successful, then continue to process the command packet
        ** If not, then exit the application in error.
        ** Note that a SB read error should not always result in an app quitting.
        */
        if (status == CFE_SUCCESS)
        {
            GENERIC_EPS_ProcessCommandPacket();
        }
        else
        {
            CFE_EVS_SendEvent(GENERIC_EPS_PIPE_ERR_EID, CFE_EVS_EventType_ERROR, "GENERIC_EPS: SB Pipe Read Error = %d", (int) status);
            GENERIC_EPS_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }
    
    /*
    ** Performance log exit stamp
    */
    CFE_ES_PerfLogExit(GENERIC_EPS_PERF_ID);

    /*
    ** Exit the application
    */
    CFE_ES_ExitApp(GENERIC_EPS_AppData.RunStatus);
} 


/* 
** Initialize application
*/
int32 GENERIC_EPS_AppInit(void)
{
    int32 status = OS_SUCCESS;
    uint8_t i = 0;
    
    GENERIC_EPS_AppData.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Register the events
    */ 
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);    /* as default, no filters are used */
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GENERIC_EPS: Error registering for event services: 0x%08X\n", (unsigned int) status);
       return status;
    }

    /*
    ** Create the Software Bus command pipe 
    */
    status = CFE_SB_CreatePipe(&GENERIC_EPS_AppData.CmdPipe, GENERIC_EPS_PIPE_DEPTH, "EPS_CMD_PIPE");
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(GENERIC_EPS_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
            "Error Creating SB Pipe,RC=0x%08X",(unsigned int) status);
       return status;
    }
    
    /*
    ** Subscribe to ground commands
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(GENERIC_EPS_CMD_MID), GENERIC_EPS_AppData.CmdPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(GENERIC_EPS_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
            "Error Subscribing to HK Gnd Cmds, MID=0x%04X, RC=0x%08X",
            GENERIC_EPS_CMD_MID, (unsigned int) status);
        return status;
    }

    /*
    ** Subscribe to housekeeping (hk) message requests
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(GENERIC_EPS_REQ_HK_MID), GENERIC_EPS_AppData.CmdPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(GENERIC_EPS_SUB_REQ_HK_ERR_EID, CFE_EVS_EventType_ERROR,
            "Error Subscribing to HK Request, MID=0x%04X, RC=0x%08X",
            GENERIC_EPS_REQ_HK_MID, (unsigned int) status);
        return status;
    }

    /* 
    ** Initialize the published HK message - this HK message will contain the 
    ** telemetry that has been defined in the GENERIC_EPS_HkTelemetryPkt for this app.
    */
    CFE_MSG_Init(CFE_MSG_PTR(GENERIC_EPS_AppData.HkTelemetryPkt.TlmHeader),
                   CFE_SB_ValueToMsgId(GENERIC_EPS_HK_TLM_MID),
                   GENERIC_EPS_HK_TLM_LNGTH);

    /* 
    ** Always reset all counters during application initialization 
    */
    GENERIC_EPS_ResetCounters();

    /*
    ** Initialize application data
    ** Note that counters are excluded as they were reset in the previous code block
    */
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.BatteryVoltage = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.BatteryTemperature = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.EPSTemperature = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.SolarArrayVoltage = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.SolarArrayTemperature = 0;
    for(i = 0; i < 8; i++)
    {
        GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.Switch[i].Voltage = 0;
        GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.Switch[i].Current = 0;
        GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK.Switch[i].Status = 0;
    }

    /*
    ** Initialize hardware interface data
    */ 
    GENERIC_EPS_AppData.Generic_epsI2C.handle = GENERIC_EPS_CFG_I2C_HANDLE;
    GENERIC_EPS_AppData.Generic_epsI2C.addr = GENERIC_EPS_CFG_I2C_ADDRESS;
    GENERIC_EPS_AppData.Generic_epsI2C.isOpen = I2C_CLOSED;
    GENERIC_EPS_AppData.Generic_epsI2C.speed = GENERIC_EPS_CFG_I2C_SPEED;

    /* Open device specific protocols */
    status = i2c_master_init(&GENERIC_EPS_AppData.Generic_epsI2C);
    if(status != OS_SUCCESS) 
    {
        CFE_EVS_SendEvent(GENERIC_EPS_I2C_INIT_ERR_EID,
                          CFE_EVS_EventType_ERROR,
                          "Could not initialize generic EPS I2C. Error = 0x%08x",
                          status);
    }
    else
    {
        /* 
        ** Send an information event that the app has initialized. 
        ** This is useful for debugging the loading of individual applications.
        */
        status = CFE_EVS_SendEvent(GENERIC_EPS_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION,
                "GENERIC_EPS App Initialized. Version %d.%d.%d.%d",
                    GENERIC_EPS_MAJOR_VERSION,
                    GENERIC_EPS_MINOR_VERSION, 
                    GENERIC_EPS_REVISION, 
                    GENERIC_EPS_MISSION_REV);	
        if (status != CFE_SUCCESS)
        {
            CFE_ES_WriteToSysLog("GENERIC_EPS: Error sending initialization event: 0x%08X\n", (unsigned int) status);
        }
    }
    return status;
} 


/* 
** Process packets received on the GENERIC_EPS command pipe
*/
void GENERIC_EPS_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_GetMsgId(GENERIC_EPS_AppData.MsgPtr, &MsgId);
    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        /*
        ** Ground Commands with command codes fall under the GENERIC_EPS_CMD_MID (Message ID)
        */
        case GENERIC_EPS_CMD_MID:
            GENERIC_EPS_ProcessGroundCommand();
            break;

        /*
        ** All other messages, other than ground commands, add to this case statement.
        */
        case GENERIC_EPS_REQ_HK_MID:
            GENERIC_EPS_ProcessTelemetryRequest();
            break;

        /*
        ** All other invalid messages that this app doesn't recognize, 
        ** increment the command error counter and log as an error event.  
        */
        default:
            GENERIC_EPS_AppData.HkTelemetryPkt.CommandErrorCount++;
            CFE_EVS_SendEvent(GENERIC_EPS_PROCESS_CMD_ERR_EID,CFE_EVS_EventType_ERROR, "GENERIC_EPS: Invalid command packet, MID = 0x%x", CFE_SB_MsgIdToValue(MsgId));
            break;
    }
    return;
} 


/*
** Process ground commands
*/
void GENERIC_EPS_ProcessGroundCommand(void)
{
    int32 status = OS_SUCCESS;
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t CommandCode = 0;


    /*
    ** MsgId is only needed if the command code is not recognized. See default case
    */
    CFE_MSG_GetMsgId(GENERIC_EPS_AppData.MsgPtr, &MsgId);   

    /*
    ** Ground Commands, by definition, have a command code (_CC) associated with them
    ** Pull this command code from the message and then process
    */
    CFE_MSG_GetFcnCode(GENERIC_EPS_AppData.MsgPtr, &CommandCode);
    switch (CommandCode)
    {
        /*
        ** NOOP Command
        */
        case GENERIC_EPS_NOOP_CC:
            /*
            ** First, verify the command length immediately after CC identification 
            ** Note that VerifyCmdLength handles the command and command error counters
            */
            if (GENERIC_EPS_VerifyCmdLength(GENERIC_EPS_AppData.MsgPtr, sizeof(GENERIC_EPS_NoArgs_cmd_t)) == OS_SUCCESS)
            {
                /* Second, send EVS event on successful receipt ground commands*/
                CFE_EVS_SendEvent(GENERIC_EPS_CMD_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "GENERIC_EPS: NOOP command received");
                /* Third, do the desired command action if applicable, in the case of NOOP it is no operation */
            }
            break;

        /*
        ** Reset Counters Command
        */
        case GENERIC_EPS_RESET_COUNTERS_CC:
            if (GENERIC_EPS_VerifyCmdLength(GENERIC_EPS_AppData.MsgPtr, sizeof(GENERIC_EPS_NoArgs_cmd_t)) == OS_SUCCESS)
            {
                CFE_EVS_SendEvent(GENERIC_EPS_CMD_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "GENERIC_EPS: RESET counters command received");
                GENERIC_EPS_ResetCounters();
            }
            break;

        /*
        ** Change switch state
        */
        case GENERIC_EPS_SWITCH_CC:
            if (GENERIC_EPS_VerifyCmdLength(GENERIC_EPS_AppData.MsgPtr, sizeof(GENERIC_EPS_Switch_cmd_t)) == OS_SUCCESS)
            {
                CFE_EVS_SendEvent(GENERIC_EPS_CMD_SWITCH_INF_EID, CFE_EVS_EventType_INFORMATION, "GENERIC_EPS: Switch command received");
                status = GENERIC_EPS_CommandSwitch(&GENERIC_EPS_AppData.Generic_epsI2C,
                                                 ((GENERIC_EPS_Switch_cmd_t*) GENERIC_EPS_AppData.MsgPtr)->SwitchNumber,
                                                 ((GENERIC_EPS_Switch_cmd_t*) GENERIC_EPS_AppData.MsgPtr)->State,
                                                   &GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK);
                if (status == OS_SUCCESS)
                {
                    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceCount++;
                    CFE_EVS_SendEvent(GENERIC_EPS_SWITCH_INF_EID, CFE_EVS_EventType_INFORMATION, "GENERIC_EPS: Switch %d set to 0x%02x", ((GENERIC_EPS_Switch_cmd_t*) GENERIC_EPS_AppData.MsgPtr)->SwitchNumber, ((GENERIC_EPS_Switch_cmd_t*) GENERIC_EPS_AppData.MsgPtr)->State);
                }
                else
                {
                    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceErrorCount++;
                    CFE_EVS_SendEvent(GENERIC_EPS_SWITCH_ERR_EID, CFE_EVS_EventType_ERROR, "GENERIC_EPS: Set switch %d to 0x%02x failed!", ((GENERIC_EPS_Switch_cmd_t*) GENERIC_EPS_AppData.MsgPtr)->SwitchNumber, ((GENERIC_EPS_Switch_cmd_t*) GENERIC_EPS_AppData.MsgPtr)->State);
                }
            }
            break;

        /*
        ** Invalid Command Codes
        */
        default:
            /* Increment the error counter upon receipt of an invalid command */
            GENERIC_EPS_AppData.HkTelemetryPkt.CommandErrorCount++;
            CFE_EVS_SendEvent(GENERIC_EPS_CMD_ERR_EID, CFE_EVS_EventType_ERROR, 
                "GENERIC_EPS: Invalid command code for packet, MID = 0x%x, cmdCode = 0x%x", CFE_SB_MsgIdToValue(MsgId), CommandCode);
            break;
    }
    return;
} 


/*
** Process Telemetry Request - Triggered in response to a telemetery request
*/
void GENERIC_EPS_ProcessTelemetryRequest(void)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t CommandCode = 0;

    /* MsgId is only needed if the command code is not recognized. See default case */
    CFE_MSG_GetMsgId(GENERIC_EPS_AppData.MsgPtr, &MsgId);   

    /* Pull this command code from the message and then process */
    CFE_MSG_GetFcnCode(GENERIC_EPS_AppData.MsgPtr, &CommandCode);
    switch (CommandCode)
    {
        case GENERIC_EPS_REQ_HK_TLM:
            GENERIC_EPS_ReportHousekeeping();
            break;

        /*
        ** Invalid Command Codes
        */
        default:
            /* Increment the error counter upon receipt of an invalid command */
            GENERIC_EPS_AppData.HkTelemetryPkt.CommandErrorCount++;
            CFE_EVS_SendEvent(GENERIC_EPS_DEVICE_TLM_ERR_EID, CFE_EVS_EventType_ERROR, 
                "GENERIC_EPS: Invalid command code for packet, MID = 0x%x, cmdCode = 0x%x", CFE_SB_MsgIdToValue(MsgId), CommandCode);
            break;
    }
    return;
}


/* 
** Report Application Housekeeping
*/
void GENERIC_EPS_ReportHousekeeping(void)
{
    int32 status = OS_SUCCESS;

    status = GENERIC_EPS_RequestHK(&GENERIC_EPS_AppData.Generic_epsI2C, &GENERIC_EPS_AppData.HkTelemetryPkt.DeviceHK);
    if (status == OS_SUCCESS)
    {
        GENERIC_EPS_AppData.HkTelemetryPkt.DeviceCount++;
    }
    else
    {
        GENERIC_EPS_AppData.HkTelemetryPkt.DeviceErrorCount++;
        CFE_EVS_SendEvent(GENERIC_EPS_REQ_HK_ERR_EID, CFE_EVS_EventType_ERROR, 
                "GENERIC_EPS: Request device HK reported error %d", status);
    }

    /* Time stamp and publish housekeeping telemetry */
    CFE_SB_TimeStampMsg((CFE_MSG_Message_t *) &GENERIC_EPS_AppData.HkTelemetryPkt);
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *) &GENERIC_EPS_AppData.HkTelemetryPkt, true);
    return;
}


/*
** Reset all global counter variables
*/
void GENERIC_EPS_ResetCounters(void)
{
    GENERIC_EPS_AppData.HkTelemetryPkt.CommandErrorCount = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.CommandCount = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceErrorCount = 0;
    GENERIC_EPS_AppData.HkTelemetryPkt.DeviceCount = 0;
    return;
} 


/*
** Verify command packet length matches expected
*/
int32 GENERIC_EPS_VerifyCmdLength(CFE_MSG_Message_t * msg, uint16 expected_length)
{     
    int32 status = OS_SUCCESS;
    CFE_SB_MsgId_t msg_id = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t cmd_code = 0;
    size_t actual_length = 0;

    CFE_MSG_GetSize(msg, &actual_length);
    if (expected_length == actual_length)
    {
        /* Increment the command counter upon receipt of an invalid command */
        GENERIC_EPS_AppData.HkTelemetryPkt.CommandCount++;
    }
    else
    {
        CFE_MSG_GetMsgId(msg, &msg_id);
        CFE_MSG_GetFcnCode(msg, &cmd_code);

        CFE_EVS_SendEvent(GENERIC_EPS_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
           "Invalid msg length: ID = 0x%X,  CC = %d, Len = %ld, Expected = %d",
              CFE_SB_MsgIdToValue(msg_id), cmd_code, actual_length, expected_length);

        status = OS_ERROR;

        /* Increment the command error counter upon receipt of an invalid command */
        GENERIC_EPS_AppData.HkTelemetryPkt.CommandErrorCount++;
    }
    return status;
} 
