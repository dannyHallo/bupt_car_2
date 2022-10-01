#pragma once

#include "math.h"

// float Kp = 0.9f, Ki = 0, Kd = 0;
// // float Kp = 1.0f, Ki = 0.0f, Kd = 0;
// float P = 0, I = 0, D = 0, PID_value = 0, error = 0;
// float previous_error = 0, previous_I = 0;
// float errorTolerance = 10.0f;

// int getPID(float error) {
//   if (abs(error) < errorTolerance) {
//     I = 0;
//   }

//   P         = error;
//   I         = I + error;
//   D         = error - previous_error;
//   PID_value = (Kp * P) + (Ki * I) + (Kd * D);
//   clamp(PID_value, -64.0f, 64.0f);

//   previous_error = error;
//   return customRound(PID_value);
// }

class pid {
public:
    pid(float kp,float ki,float kd) {
        Kp = kp;
        Ki = ki;
        Kd = kd;
    }

    float update(float error,float t = 1) {
        P = error;
        I = I+error;
        D = error-previous_error;
        PID_value = ((Kp*P)+(Ki*I)+(Kd*D))*t;

        previous_error = error;
        return PID_value;
    }

private:
    float Kp = 0.9f,Ki = 0.1,Kd = 0.1;
    float P = 0,I = 0,D = 0,PID_value = 0;
    float previous_error = 0;
};