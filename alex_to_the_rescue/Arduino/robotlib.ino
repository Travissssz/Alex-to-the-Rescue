#include <AFMotor.h>
// Direction values


// Motor control
#define FRONT_LEFT   4 // M4 on the driver shield
#define FRONT_RIGHT  1 // M1 on the driver shield
#define BACK_LEFT    3 // M3 on the driver shield
#define BACK_RIGHT   2 // M2 on the driver shield

AF_DCMotor motorFL(FRONT_LEFT);
AF_DCMotor motorFR(FRONT_RIGHT);
AF_DCMotor motorBL(BACK_LEFT);
AF_DCMotor motorBR(BACK_RIGHT);

void move(int direction)//float speed, int direction)
{
  int speed_scaled = 255; //(speed / 100.0) * 255;
  motorFL.setSpeed(speed_scaled);
  motorFR.setSpeed(speed_scaled);
  motorBL.setSpeed(speed_scaled);
  motorBR.setSpeed(speed_scaled);

  switch (direction)
  {
    case BACK:
      motorFL.run(BACKWARD);
      motorFR.run(BACKWARD);
      motorBL.run(FORWARD);
      motorBR.run(FORWARD);
      break;
    case GO:
      motorFL.run(FORWARD);
      motorFR.run(FORWARD);
      motorBL.run(BACKWARD);
      motorBR.run(BACKWARD);
      break;
    case CW:
      motorFL.run(BACKWARD);
      motorFR.run(FORWARD);
      motorBL.run(FORWARD);
      motorBR.run(BACKWARD);
      break;
    case CCW:
      motorFL.run(FORWARD);
      motorFR.run(BACKWARD);
      motorBL.run(BACKWARD);
      motorBR.run(FORWARD);
      break;
    case STOP:
    default:
      motorFL.run(RELEASE);
      motorFR.run(RELEASE);
      motorBL.run(RELEASE);
      motorBR.run(RELEASE);
  }
}

void forward()//float dist, float speed)
{
  /*
  if (dist > 0) {
    deltaDist = dist;
  } else {
    deltaDist = 9999999;
  }
  newDist = forwardDist + deltaDist;
  */
dir = (TDirection) FORWARD;
move(FORWARD);
delay(200);
stop();
}

void nudgeForward()
{
  dir = (TDirection) FORWARD;
  move(FORWARD);
  delay(100);
  stop();
}

void speedForward()
{
  dir = (TDirection) FORWARD;
  move(FORWARD);
  delay(500);
  stop();
}


void backward()//float dist, float speed)
{
    /*if (dist > 0) {
    deltaDist = dist;
  } else {
    deltaDist = 9999999;
  }
  newDist = reverseDist + deltaDist;
  */
  dir = (TDirection) BACKWARD;
  move(BACKWARD);
  delay(200);
  stop();
}

void nudgeBackward()
{
  dir = (TDirection) BACKWARD;
  move(BACKWARD);
  delay(100);
  stop();
}

void speedBackward()
{
  dir = (TDirection) BACKWARD;
  move(BACKWARD);
  delay(500);
  stop();
}

void ccw()//float dist, float speed)
{
  dir = (TDirection) CCW;
  move(CCW);
  delay(200);
  stop();
}

void speedccw()
{
  dir = (TDirection) CCW;
  move(CCW);
  delay(2000);
  stop();
  
}

void cw()//float dist, float speed)
{
  dir = (TDirection) CW;
  move(CW);
  delay(200);
  stop();
}

void speedcw()//float dist, float speed)
{
  dir = (TDirection) CW;
  move(CW);
  delay(2000);
  stop();
}

void stop()
{
  dir = (TDirection) STOPS;
  move(STOP);
}
