#include "DataProcessing.h"

float PID_Control(PI_str* pPI, float Target, float Present){
    pPI->Error = Target - Present;
    uint8_t ui_flag = !(((pPI->Out_temp > pPI->Max) || (pPI->Out_temp < -pPI->Max)) && (pPI->Out_temp * pPI->Error >= 0));
    
    pPI->up = pPI->Kp * pPI->Error;
    pPI->ui = pPI->ui + pPI->Ki * pPI->Error * ui_flag;
    
    pPI->Out_temp = pPI->up + pPI->ui;
    
    float PIout = 0;
    
    if(pPI->Out_temp > pPI->Max)
        PIout = pPI->Max;
    else if(pPI->Out_temp < -pPI->Max)
        PIout = -pPI->Max;
    else 
        PIout = pPI->Out_temp;
    
    return PIout;
}

float PI_Control_Err(PI_str* pPI, float Error){
    uint8_t ui_flag = !(((pPI->Out_temp > pPI->Max) || (pPI->Out_temp < -pPI->Max)) && (pPI->Out_temp * Error >= 0));
    
    pPI->up = pPI->Kp * Error;
    pPI->ui = pPI->ui + pPI->Ki * Error * ui_flag;
    
    pPI->Out_temp = pPI->up + pPI->ui;
    
    float PIout = 0;
    
    if(pPI->Out_temp > pPI->Max)
        PIout = pPI->Max;
    else if(pPI->Out_temp < -pPI->Max)
        PIout = -pPI->Max;
    else 
        PIout = pPI->Out_temp;
    
    return PIout;
}

float ObsPID_Control(PI_str* pPI, float Target, float Present){
    pPI->Error = Target-Present;

    uint8_t ui_flag = !(((pPI->Out_temp > pPI->Max) || (pPI->Out_temp < -pPI->Max)) && (pPI->Out_temp * pPI->Error >= 0));
    
    pPI->up = pPI->Kp * pPI->Error;
    pPI->ui = pPI->ui + pPI->Ki * pPI->Error * ui_flag;
    
    pPI->Out_temp = pPI->up + pPI->ui;
    
    float PIout = 0;
    
    if(pPI->Out_temp > pPI->Max)
        PIout = pPI->Max;
    else if(pPI->Out_temp < -pPI->Max)
        PIout = -pPI->Max;
    else 
        PIout = pPI->Out_temp;
    
    return PIout;
}

float PIMAX_Control(PI_str* pPI, float Target, float Present, float MaxUp, float MaxDown){
    float Error = Target - Present;
    uint8_t ui_flag = !(((pPI->Out_temp > MaxUp) || (pPI->Out_temp < MaxDown)) && (pPI->Out_temp * Error >= 0));
    
    pPI->up = pPI->Kp * Error;
    pPI->ui = pPI->ui + pPI->Ki * Error * ui_flag;
    
    pPI->Out_temp = pPI->up + pPI->ui;
    
    float PIout = 0;
    
    if(pPI->Out_temp > MaxUp)
        PIout = MaxUp;
    else if(pPI->Out_temp < MaxDown)
        PIout = MaxDown;
    else 
        PIout = pPI->Out_temp;
    
    return PIout;
}

void LPF(float* Uo, float Ui, float Fs, float Wc){
    *Uo += Wc / Fs * (Ui - *Uo);
}

float SMOSwitchFunction1(float E, float Error){
    float SF_Out;

    SF_Out = Error / E;
    
    if(SF_Out > 1)
        SF_Out = 1;
    else if(SF_Out < -1)
        SF_Out = -1;

    return SF_Out;
}

void SlidingModeObserver(ControlCommand_str* CtrlCom, MotorParameter_str* MotorParameter, MotorRealTimeInformation_str* MRT_Inf, SlidingModeObserver_str* SMO){
    MRT_Inf->EMF_Peak = MRT_Inf->Spd * MotorParameter->Np * MotorParameter->Flux;
    MRT_Inf->EMF_Rms = MRT_Inf->EMF_Peak / 1.732f * 1.414f;
    MRT_Inf->Ex = -MRT_Inf->EMF_Rms * MRT_Inf->SinTheta;
    MRT_Inf->Ey =  MRT_Inf->EMF_Rms * MRT_Inf->CosTheta;

    SMO->Vx = SMOSwitchFunction1(SMO->E1, SMO->Ix - MRT_Inf->Ix);
    SMO->Vy = SMOSwitchFunction1(SMO->E1, SMO->Iy - MRT_Inf->Iy);

    SMO->Ix = SMO->Ix + CtrlCom->CurTs * (MRT_Inf->Ux - MotorParameter->Rs * SMO->Ix - SMO->Ex - SMO->h1 * SMO->Vx) / MotorParameter->Ls;
    SMO->Iy = SMO->Iy + CtrlCom->CurTs * (MRT_Inf->Uy - MotorParameter->Rs * SMO->Iy - SMO->Ey - SMO->h1 * SMO->Vy) / MotorParameter->Ls;

    SMO->Ex = SMO->Ex + CtrlCom->CurTs * (-SMO->SpdE * SMO->Ey + SMO->h2 * SMO->Vx / MotorParameter->Ls);
    SMO->Ey = SMO->Ey + CtrlCom->CurTs * ( SMO->SpdE * SMO->Ex + SMO->h2 * SMO->Vy / MotorParameter->Ls);
    
    if(SMO->Ex > SMO->Switch_EMF * 0.25f){
        SMO->QuadDec_X = 1;
    }
    else if(SMO->Ex < -SMO->Switch_EMF * 0.25f){
        SMO->QuadDec_X = 0;
    }
    
    if(SMO->Ey > SMO->Switch_EMF * 0.25f){
        SMO->QuadDec_Y = 1;
        if(SMO->QuadDec_Y_temp == 0){
            SMO->EMF_Dir = (SMO->QuadDec_X)?(1):(-1);
        }    
    }
    else if(SMO->Ey < -SMO->Switch_EMF * 0.25f){
        SMO->QuadDec_Y = 0;
    }
    
    SMO->QuadDec_Y_temp = SMO->QuadDec_Y;
    
    SMO->EMF_Rms = sqrtf(SMO->Ex * SMO->Ex + SMO->Ey * SMO->Ey);
    SMO->EMF_Rms2 =  -SMO->Ex * SMO->SinTheta + SMO->Ey * SMO->CosTheta;
    SMO->EMF_Peak = SMO->EMF_Rms * 1.732f / 1.414f;

    Cordic(SMO->ThetaE, &(SMO->SinTheta), &(SMO->CosTheta));
    
    switch(SMO->status){
        case 0:
            if(SMO->EMF_Rms > SMO->Switch_EMF * 0.5f){
                SMO->status = 1;
            }
            else{
                SMO->status = 0;
            }
            break;
        case 1:
            if(SMO->EMF_Rms < SMO->Switch_EMF * 0.25f){
                SMO->status = 0;
            }
            else if(SMO->EMF_Rms > SMO->Switch_EMF){
                SMO->status = 2;
            }
            else{
                SMO->status = 1;
            }
            break;
        case 2:
            if(SMO->EMF_Rms < SMO->Switch_EMF * 0.25f){
                SMO->status = 0;
            }
            else{
                SMO->status = 2;
            }
            break;
    }
    
    float SpdE = 0;
    float ThetaE_temp = 0;
    
    switch(SMO->status){
        case 0:
            LPF(&(SMO->SpdE), 0, CtrlCom->CurFs, SMO->Spd_LPF_wc);
        
            ThetaE_temp = SMO->ThetaE + SpdE * CtrlCom->CurTs;
            if(ThetaE_temp < 0)
                ThetaE_temp += 2 * PI;
            SMO->ThetaE = fmodf(ThetaE_temp, 2 * PI);
            break;
        case 1:
            SMO->de = (-SMO->Ex * SMO->CosTheta - SMO->Ey * SMO->SinTheta) / SMO->Switch_EMF * SMO->EMF_Dir;
            
            SpdE = PI_Control_Err(&(SMO->SpdE_PI), SMO->de);
            LPF(&(SMO->SpdE), SpdE, CtrlCom->CurFs, SMO->Spd_LPF_wc);
        
            ThetaE_temp = SMO->ThetaE + SpdE * CtrlCom->CurTs;
            if(ThetaE_temp < 0)
                ThetaE_temp += 2 * PI;
            SMO->ThetaE = fmodf(ThetaE_temp, 2 * PI);
            SMO->ThetaE2 = atanf(-SMO->Ex / SMO->Ey);
            break;
        case 2:
            SMO->de = (-SMO->Ex * SMO->CosTheta - SMO->Ey * SMO->SinTheta) / SMO->EMF_Rms * SMO->EMF_Dir;
            
            SpdE = PI_Control_Err(&(SMO->SpdE_PI), SMO->de);
            LPF(&(SMO->SpdE), SpdE, CtrlCom->CurFs, SMO->Spd_LPF_wc);
        
            ThetaE_temp = SMO->ThetaE + SpdE * CtrlCom->CurTs;
            if(ThetaE_temp < 0)
                ThetaE_temp += 2 * PI;
            SMO->ThetaE = fmodf(ThetaE_temp, 2 * PI);
            SMO->ThetaE2 = atanf(-SMO->Ex / SMO->Ey);
            break;
    }
    
    if((SMO->SpdE < 0.1f) && (SMO->SpdE > -0.1f))
        SMO->Flux = 0;
    else{
        LPF(&(SMO->Flux), fabsf(SMO->EMF_Peak / SMO->SpdE), CtrlCom->CurFs, 5 * 2 * PI);
    }
}

void CtrlComFilter(float *Target, float CtrlCom, float TickAdd){
    if(*Target < CtrlCom){
        if(*Target + TickAdd > CtrlCom){
            *Target = CtrlCom;
        }
        else{
            *Target += TickAdd;
        }
    }
    else if(*Target > CtrlCom){
        if(*Target - TickAdd < CtrlCom){
            *Target = CtrlCom;
        }
        else{
            *Target -= TickAdd;
        }
    }          
}
