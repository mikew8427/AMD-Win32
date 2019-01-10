#include "windows.h"
#include <math.h>
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include "jdate.h"

float JulianOf(char *date)
{
float y,m,d,w;
	sscanf(date,"%f/%f/%f",&y,&m,&d);
	w=y+(m-(float)2.85)/(float)12;
	return((float)floor((float)floor((float)floor((float)367.0*w)-floor(w)-(float)0.75*(float)floor(w)+d)-(float)0.75*(float)2.0)+(float)1721115.0);
}
BOOL DateOfJulian(float jul,char *dbuf)
{
int y=0,m=0,d=0;
float n1=0,n2=0,y1=0,m1=0;

n1=(jul-(float)1721119.0)+(float)2.0;
y1=(float)floor((n1-(float)0.2)/(float)365.25);
n2=n1-(float)floor((float)365.25*y1);
m1=(float)floor((n2-(float)0.5)/(float)30.6);
d=(int)(n2-(float)30.6*m1+(float)0.5);
y=(int)(y1+(m1>9));
m=(int)(m1+3-((m1>9)*12));
sprintf(dbuf,"%.4d/%.2d/%.2d",y,m,d);
return TRUE;
}
