#include "ccd.h"
#include "gpio.h"
#include "adc.h"
#include "delay.h"
#include "uart.h"
#include "pit.h"
#include "oled.h"
#include "ftm.h"
#include "define.h"
#include "chuinit.h"


unsigned char AverageValue;
/*************************************************************************
*
*  函数名称：void DelayNs(void)
*  功能说明：CCD延时程序 200ns
*  参数说明：
*  函数返回：无
*  修改时间：
*  备    注：
*************************************************************************/
void DelayNs(void)
{	
	uint8_t i;
	for(i = 0;i < 1;i++)
	{
		__nop();
		__nop();
	}
}
/*************************************************************************
*
*  函数名称：PixelAverage
*  功能说明：求数组的均值程序
*  参数说明：
*  函数返回：无
*  修改时间：
*  备    注：
*************************************************************************/
uint8_t PixelAverage(uint8_t len, uint8_t *data) 
{
  uint8_t i;
  unsigned int sum = 0;
  for(i = 0; i<len; i++) {
    sum = sum + *data++;
  }
  return ((uint8_t)(sum / len));
}


/*************************************************************************
*
*  函数名称：StartIntegration
*  功能说明：CCD启动程序
*  参数说明：
*  函数返回：无
*  修改时间：
*  备    注：
*************************************************************************/
void StartIntegration(void)
{
	uint8_t i;
	
	CCD_SI = 1;
	DelayNs();
	CCD_CLK = 1;
	DelayNs();
	CCD_SI = 0;
	DelayNs();
	CCD_CLK = 0;

	for(i=0;i<127;i++)
	{
		DelayNs();
		DelayNs();
		CCD_CLK = 1;
		DelayNs();
		DelayNs();
		CCD_CLK = 0;
	}		
	DelayNs();
  DelayNs();
  CCD_CLK = 1;           /* CLK = 1 */
  DelayNs();
  DelayNs();
  CCD_CLK = 0;         /* CLK = 0 */
}

/*************************************************************************
*
*  函数名称：ImageCapture
*  功能说明：CCD采样程序
*  参数说明：* ImageData   采样数组
*  函数返回：无
*  修改时间：
*  备    注：
*ImageData =  ad_once(ADC1, AD6a, ADC_8bit);
*************************************************************************/
void ImageCapture(uint8_t * ImageData) 
{
    uint8_t i;
    extern uint8_t AtemP ;

    CCD_SI=1;            /* SI  = 1 */
    DelayNs();
		CCD_CLK=1;           /* CLK = 1 */
    DelayNs();
    CCD_SI=0;            /* SI  = 0 */
    DelayNs();

    //Delay 10us for sample the first pixel
    /**/
		DelayUs(10);

    //Sampling Pixel 1

    *ImageData =  ADC_GetConversionValue(ADC1_SE10_PB4);
    ImageData ++ ;
    CCD_CLK=0;            /* CLK = 0 */

    for(i=0; i<127; i++) 
		{
      DelayNs();
      DelayNs();
      CCD_CLK=1;        /* CLK = 1 */
      DelayNs();
      DelayNs();

      *ImageData =  ADC_GetConversionValue(ADC1_SE10_PB4);
      ImageData ++ ;
      CCD_CLK=0;       /* CLK = 0 */
    }
    DelayNs();
    DelayNs();
    CCD_CLK=1;           /* CLK = 1 */
    DelayNs();
    DelayNs();
    CCD_CLK=0;           /* CLK = 0 */
}

/*************************************************************************
*
*  函数名称：CalculateIntegrationTime
*  功能说明：计算曝光时间
*  参数说明：
*  函数返回：无
*  修改时间：
*  备    注：
*************************************************************************/
/* 曝光时间，单位ms */
uint8_t IntegrationTime = 100;
void CalculateIntegrationTime(void)
{						
	/* 128个像素点的平均AD值 */
	uint8_t PixelAverageValue;							
	/* 128个像素点的平均电压值的10倍 */
	uint8_t PixelAverageVoltage;						
	/* 设定目标平均电压值，实际电压的10倍 */
	uint16_t TargetPixelAverageVoltage = 23;			
	/* 设定目标平均电压值与实际值的偏差，实际电压的10倍 */
	uint16_t PixelAverageVoltageError = 0;				
	/* 设定目标平均电压值允许的偏差，实际电压的10倍 */
	uint16_t TargetPixelAverageVoltageAllowError = 2;

  /* 计算128个像素点的平均AD值 */
  PixelAverageValue = PixelAverage(128,Pixel);
  /* 计算128个像素点的平均电压值,实际值的10倍 */
  PixelAverageVoltage = (uint8_t)((int)PixelAverageValue * 26 / 200);

  PixelAverageVoltageError = TargetPixelAverageVoltage - PixelAverageVoltage;
  if(PixelAverageVoltageError < -TargetPixelAverageVoltageAllowError) IntegrationTime--;
  if(PixelAverageVoltageError > TargetPixelAverageVoltageAllowError)  IntegrationTime++;
  if(IntegrationTime <= 1)	 IntegrationTime = 1;
  if(IntegrationTime >= 100) IntegrationTime = 100;
}


/***********************************************************************************************
 功能：oled采集
 形参：0
 返回：0
 详解：将ccd的AD值转换成oled显示的值
************************************************************************************************/
void oled_collect()
{
	uint8_t i,j,flag,flag1;
	for(i = 0;i <= 127;i++)
	{
		flag  = (255 - Pixel[i]) / 32;
		flag1 = (255 - Pixel[i]) % 32 / 4;
	  for(j = 7;j >= 4;j--)
			{
				if(j >  flag) oled_data[j][i]=0xff;
				if(j == flag) oled_data[j][i]=0xff<<flag1;
				if(j <  flag) oled_data[j][i]=0x00;
			}
		}
}

/***********************************************************************************************
 功能：oled显示
 形参：0
 返回：0
 详解：0
************************************************************************************************/

void oled_show()
{
	uint8_t i,j;
	for(i = 7;i >= 4;i--)
	{
		OLED_Set_XY(0,i);
	  for(j = 0;j <= 127;j++) OLED_WrDat(oled_data[i][j]);
	}
}
/*****************************
*
*  函数名称CCD_HeiXian
*  功能说明：采集黑线位置进行黑线点数计算
*  参数说明：
*  函数返回：无
*  修改时间：
*  备    注：
*************************************************************************/
void CCD_HeiXian(void)
{
	uint8_t i,value_max,value_min,x1,x2;
	
	value_max = Pixel[0];
	for (i = 2;i < 126;i++) 
  {
    if (value_max <= Pixel[i]) value_max = Pixel[i];
  }
	value_min = Pixel[0];
	for (i = 2;i < 126;i++) 
  {
    if (value_min >= Pixel[i]) value_min = Pixel[i];
  }
	AverageValue = (value_max + value_min * 2) / 3;

	for (i = saoxian;i <= 122;i++)
	{
		Right = i;
		if (Pixel[i - 2] - Pixel[i] >= 8 && Pixel[i - 3] - Pixel[i - 1] >= 8 && Pixel[i] <= AverageValue) break;
	}
	for (i = saoxian;i >= 6;i--)
	{
		Left = i;
		if (Pixel[i + 2] - Pixel[i] >= 8 && Pixel[i + 3] - Pixel[i + 1] >= 8 && Pixel[i] <= AverageValue) break;
	}
	if (Left != 6 && Right != 122 && leixin == 0) Budao = (Right - Left) / 2;
	
/***********************buxianchuli*************************************/
	
	if (Left != 6 && Right == 122)
	{
		for (i = saoxian; i <= 122; i++)
		{
			Right = i;
			if (Pixel[i - 4] - Pixel[i] >= 8 && Pixel[i - 5] - Pixel[i - 1] >= 8 && Pixel[i] <= AverageValue) break;
		}
		if (Right != 122) Center = (Right + Left) / 2;
		else
		{
			Center = Left + Budao;
			if (Center > 128) Center = 128;
			if (Center < (Right + Left) / 2) Center = (Right + Left) / 2;
		}
	}

	if (Left == 6 && Right != 122)
	{
		for (i = saoxian; i >= 6; i--)
		{
			Left = i;
			if (Pixel[i + 4] - Pixel[i] >= 8 && Pixel[i + 5] - Pixel[i + 1] >= 8 && Pixel[i] <= AverageValue) break;
		}
		if (Left != 6) Center = (Right + Left ) / 2;
		else
		{
			if (Right >= Budao) Center = Right - Budao;
			else Center = 0;
			if (Center > (Right + Left) / 2) Center = (Right + Left) / 2;
		}
	}
	
	if (Left != 6 && Right != 122) Center = (Right + Left) / 2;
	
	/*************************heixian*****************************************/
	
	if (Left == 6 && Right == 122 && AverageValue <= 85)
	{
		if (CenterLast[0] <= 63) Center = 10;
		if (CenterLast[0] >  63) Center = 110;
	}
	
	/***************************shizhi*********************************************/

	if (AverageValue >= a4 && Left <= 8 && Right >= 120 && Cross_flag_wan == 1 && AverageValue <= 200)
	{
		Cross_flag = 0;
		Cross_flag_time = 0;
	}
	if (AverageValue >= a4 && Left <= 8 && Right >= 120 && Cross_flag_wan == 0 && AverageValue <= 200)
	{
		Cross_flag = 1;
		Cross_flag_time = 0;
	}
	if (Cross_flag)
	{
		x1 = x2 = a3;
		if (leixin == 1) Cross_flag_wan = 1;
		if (Cross_flag_time >= 20)
		{
			if (leixin == 0 && leixin_last[0] == 0) 
			{
				Cross_flag_wan = 0;
				Cross_flag 		 = 0;
			}
		}
	}
	else
	{
		x1 = a1;
		x2 = a2;
		if (leixin == 0) Cross_flag_wan = 0;
	}
	
	/*******************************leixin************************************/	
	
	if (Center > 44 && Center < 88 && CenterLast[0] > 44 && CenterLast[0] < 88)
	{
		leixin = 0;//zhidao 
		jiansuflag = 0;
	}
	
	if ((Center <= 44 && Left <= 13) || (Center >=84 && Right >= 115))
	{
		leixin = 1;//wandao
		jiasuflag = 0;
	}
	
	/****************************************************************************/
	
	CenterLast[1] = CenterLast[0];
	CenterLast[0] = Center;

	leixin_last[0] = leixin;
	
	if (Center  >= 122)   Center = 122;
	if (Center  <= 6) 	  Center = 6;
	
	saoxian = Center;
	
	if (Cross_flag)
	{
		if (Center <= 63) FTMDuty = 4890 - (63 - Center) * x1;
		if (Center >= 64) FTMDuty = 4890 + (Center - 64) * x2;
		
		if (FTMDuty <= 4000) FTMDuty = 4000;
		if (FTMDuty >= 5800) FTMDuty = 5800;
	}
	else
	{
		if (Center <= 63) FTMDuty = 4890 - (63 - Center)*(63 - Center) * x1 / 100;
		if (Center >= 64) FTMDuty = 4890 + (Center - 64)*(Center - 64) * x2 / 100;
		
		if (FTMDuty <= 4000) FTMDuty = 4000;
		if (FTMDuty >= 5800) FTMDuty = 5800;
	}
	FTM_PWM_ChangeDuty(FTM0_CH4_PA7,FTMDuty);
}

/*************************************************************************
*
*  函数名称OLED_ZhongXian
*  功能说明：显示中线的位置
*  参数说明：
*  函数返回：无
*  修改时间：
*  备    注：
*************************************************************************/
void OLED_ZhongXian(void)
{
	uint8_t m;
	for (m = 0;m < 3;m++)
	{
		OLED_Set_XY(Center,m);
		OLED_WrDat(0xff);
	}
	for (m = 0;m < 3;m++)
	{
		OLED_Set_XY(Center,m);
		OLED_WrDat(0x00);
	}
}

