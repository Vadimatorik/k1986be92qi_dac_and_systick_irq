#include "1986be9x_config.h"
#include "1986BE9x.h"
#include "1986BE9x_uart.h"
#include "1986BE9x_port.h"
#include "1986BE9x_rst_clk.h"
#include "1986BE9x_it.h"
#include "mlt_lcd.h"
#include "MilFlash.h"

//---------------------------------------------------------
//ЦАП.
//---------------------------------------------------------
#define PCLK_EN(DAC)             (1<<18)                                //Маска включения тактирования ЦАП. 
#define CFG_Cfg_ON_DAC0          (1<<2)                                 //Маска включения ЦАП1.                    
#define CFG_Cfg_ON_DAC1          (1<<3)
void ADC_Init (void)
{
	RST_CLK->PER_CLOCK |= PCLK_EN(DAC);             //Включаем тактирование ЦАП.
	DAC->CFG = CFG_Cfg_ON_DAC1;                     //Включаем ЦАП2. Ассинхронно. От внутреннего источника.
}

/*
void Led_init (void)
{
  RST_CLK->PER_CLOCK |=(1<<23);                   //Включаем тактирование порта C.
	PORTC->OE |= 1;                                 //Порт - выход.
	PORTC->ANALOG |= 1;                             //Порт - цифоровой. 
	PORTC->PWR |= 1;                                //Порт - медленный режим.
}
*/
#define CLKSOURCE (1<<2)                          //Указывает источник синхросигнала: 0 - LSI, 1 - HCLK.
#define TCKINT    (1<<1)                          //Разрешает запрос на прерывание от системного таймера.
#define ENABLE    (1<<0)                          //Разрешает работу таймера.

/*
void Init_SysTick_milli (void)                    //Прерывание раз в миллисекунду. 
{
	SysTick->LOAD = (8000000/1000)-1;              
	SysTick->CTRL |= CLKSOURCE|TCKINT|ENABLE;
}*/


void Init_SysTick (void)                          //Прерывание 10000000 раз в секунду. 
{
	SysTick->LOAD = (80000000/1000000)-1;                 
	SysTick->CTRL |= CLKSOURCE|TCKINT|ENABLE;
}

const uint16_t C_4[100] = {2047, 2176, 2304, 2431, 2556, 2680, 2801, 2919, 3033, 3144, 3250, 3352, 3448, 3539, 3624, 3703, 3775, 3841, 3899, 3950, 3994, 4030, 4058, 4078, 4090, 4094, 4090, 4078, 4058, 4030, 3994, 3950, 3899, 3841, 3775, 3703, 3624, 3539, 3448, 3352, 3250, 3144, 3033, 2919, 2801, 2680, 2556, 2431, 2304, 2176, 2047, 1918, 1790, 1663, 1538, 1414, 1293, 1175, 1061, 950, 844, 742, 646, 555, 470, 391, 319, 253, 195, 144, 100, 64, 36, 16, 4, 0, 4, 16, 36, 64, 100, 144, 195, 253, 319, 391, 470, 555, 646, 742, 844, 950, 1061, 1175, 1293, 1414, 1538, 1663, 1790, 1918};
volatile uint16_t Loop = 0;
volatile uint32_t Delay_dec = 0;                  //Прерывание от SysTick таймера.
void SysTick_Handler (void)
{
	Delay_dec++; if (Delay_dec==(38-1))
	{
    DAC->DAC2_DATA = C_4[Loop];
    if (Loop<99) Loop++; else Loop = 0;
		Delay_dec=0;
	}
}
	//if (Delay_dec) Delay_dec--;
void Delay (uint32_t Delay_Data)                  //Функция задержки на основе SysTick таймера. 
{
	Delay_dec = Delay_Data;
	while (Delay_dec) {};
}

#define HCLK_SEL(CPU_C3)       (1<<8)
#define CPU_C1_SEL(HSE)        (1<<1)
#define CPU_C2_SEL(CPU_C2_SEL) (1<<2)
#define PCLK_EN(RST_CLK)       (1<<4)
#define HS_CONTROL(HSE_ON)     (1<<0)
#define REG_0F(HSI_ON)        ~(1<<22)
#define RTC_CS(ALRF)           (1<<2)
#define PCLK(BKP)              (1<<27)

#define RST_CLK_ON_Clock()       RST_CLK->PER_CLOCK |= PCLK_EN(RST_CLK)                 //Включаем тактирование контроллера тактовой частоты (по умолчанию включено).
#define HSE_Clock_ON()           RST_CLK->HS_CONTROL = HS_CONTROL(HSE_ON)               //Разрешаем использование HSE генератора. 
#define HSE_Clock_OffPLL()       RST_CLK->CPU_CLOCK  = CPU_C1_SEL(HSE)|HCLK_SEL(CPU_C3);//Настраиваем "путь" сигнала и включаем тактирование от HSE генератора.

#define PLL_CONTROL_PLL_CPU_ON  (1<<2)                                                  //PLL включена. 
#define PLL_CONTROL_PLL_CPU_PLD (1<<3)                                                  //Бит перезапуска PLL.
void HSE_PLL (uint8_t PLL_multiply)                                                              //Сюда передаем частоту в разах "в 2 раза" например. 
{
	RST_CLK->PLL_CONTROL  = RST_CLK->PLL_CONTROL&(~(0xF<<8));                                      //Удаляем старое значение.
	RST_CLK->PLL_CONTROL |= PLL_CONTROL_PLL_CPU_ON|((PLL_multiply-1)<<8)|PLL_CONTROL_PLL_CPU_PLD;  //Включаем PLL и включаем умножение в X раз, а так же перезапускаем PLL.
	RST_CLK->CPU_CLOCK   |= HCLK_SEL(CPU_C3)|CPU_C2_SEL(CPU_C2_SEL)|CPU_C1_SEL(HSE);               //Настриваем "маршрут" частоты через PLL и включаем тактирование от HSE.
}

/*
void Block (void)                                //Подпрограмма ожидания (защиты).
{
	PORTC->RXTX |= 1;
	Delay_ms (1000);
	PORTC->RXTX = 0;
	Delay_ms (1000);
}*/




//---------------------------------------------------------
//Настраиваем выход, подключенный к усилителю. 
//---------------------------------------------------------
#define PER_CLOCK_PORTE              (1<<25)      //Бит включения тактирования порта E.
#define PORT_OE_OUT_PORTE_0          (1<<0)       //Включение этого бита переводит PORTE_0 в "выход". 
#define ANALOG_EN_DIGITAL_PORTE_0    (1<<0)       //Включаем цифровой режим бита порта PORTE_0.
#define PWR_MAX_PORTE_0              (3<<0)       //Включение данных бит переключает PORTE_0 в режим максимальной скорости.

#define PORT_RXTX_PORTE_0_OUT_1      (1<<0)       //Маска порта для подачи "1" на выход.

void Buzzer_out_init (void)
{
	RST_CLK->PER_CLOCK |= PER_CLOCK_PORTE;          //Включаем тактирование порта E.
	PORTE->OE |= PORT_OE_OUT_PORTE_0;               //Выход. 
	PORTE->ANALOG |= ANALOG_EN_DIGITAL_PORTE_0;     //Цифровой.
	PORTE->PWR |= PWR_MAX_PORTE_0;                  //Максимальная скорость (около 10 нс).
}

void Buzzer_out_DAC_init (void)
{
	RST_CLK->PER_CLOCK |= PER_CLOCK_PORTE;          //Включаем тактирование порта E.
	PORTE->OE |= PORT_OE_OUT_PORTE_0;               //Выход. 
	PORTE->ANALOG = 0;                              //Аналоговый.
	PORTE->PWR |= PWR_MAX_PORTE_0;                  //Максимальная скорость (около 10 нс).
}

const uint32_t MES[13] = {191, 180, 170, 161, 152, 143, 135, 128, 120, 114, 107, 101, 96};
int main (void)
{
  HSE_Clock_ON();                                  //Разрешаем использование HSE генератора. 
  HSE_Clock_OffPLL();                              //Настраиваем "путь" сигнала и включаем тактирование от HSE генератора.
  Buzzer_out_DAC_init();                           //Настраиваем порт для ЦАП.
  ADC_Init();                                      //Настраиваем ЦАП.
  HSE_PLL(10);                                     //8 Мгц -> 80 Мгц. 
  Init_SysTick();                                  //Инициализируем системный таймер для прерываний.
  while (1)                                        
  {

  }
}
   /* */

/*
	const uint32_t MES[12] = {191, 180, 170, 161, 152, 143, 135, 128, 120, 114, 107, 101};
int main (void)
{
	Buzzer_out_init();                               //Инициализируем пин, пин звукового генератора. 
	Init_SysTick_micro();                            //Инициализируем системный таймер для счета мллисекунд. 
  while (1)                                        //Создаем колебания с частотой 130.82 гц.
	{
		
		}
	}
	
	
}
*/




	//Init_SysTick();                               //Инициализируем системный таймер. 
	
	//Led_init();                                     //Инициализируем ножку 0 порта C для светодиода. 
	//Block();                                        //Подпрограмма ожидания (защиты).
	//HSE_Clock_ON();                                 //Разрешаем использование HSE генератора. 
	//HSE_PLL(1);                                     //Включаем тактирование с умножением 2
	
	//uint8_t PLL_Data = 1;                           //Здесь храним коэффициент умножения. 
	