#ifndef __DFSERIAL
#define __DFSERIAL

class DFSerial
{
public:
	static int Init();
	static void Halt();
	//static void Light(int i);
	static int ReduceSteps(int increase_value=0); // ваше время истекло...
	
protected:
	static int light, last_light;
};

#endif