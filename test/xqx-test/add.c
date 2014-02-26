/*
 * =====================================================================================
 *
 *       Filename:  add.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年02月18日 22时24分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Xiao Qixue (XQX), xiaoqixue_1@163.com
 *   Organization:  Tsinghua University
 *
 * =====================================================================================
 */
#include "stdio.h"

int add( int a, int b) 
{
	return a + b;
}


int main(int argc, char* argv[])
{

	if( argc < 3) 
		return -1;

	int x, y;
	x = atoi(argv[1]);
	y = atoi(argv[2]);

	int z ;
	z = add(x, y);

	printf("x+y=%d", z);




}
