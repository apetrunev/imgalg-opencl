#ifndef CL_BLUR_H_
#define CL_BLUR_H_

int gauss5[] = { 
	1, 4, 7, 4, 1,	
	4, 20, 33, 20, 4,
	7, 33, 55, 33, 7,
	4, 20, 33, 20, 4,
	1, 4, 7, 4, 1
};

int gauss5_summ = 331;

#endif /* CL_BLUR_H_ */
