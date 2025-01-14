/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "i2c-lcd.h"
#include "key.h"
#include "hc_sr04.h"
#include "funciones.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define tres_s 500

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile char key = '\0';
volatile uint8_t posicion;
volatile uint8_t destino;
volatile uint8_t flag_tiempoTrans = 0; //Flag usado para borrar la tecla tras un tiempo de transmisión


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void actualizaInfoSPI (void);
void enviaSPI (void);
void gestorTemperatura (void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t distancia = 0;
char buf_lcd[18];
volatile uint8_t cicloEnvio = 0;

volatile uint8_t contador = 0;

uint8_t envioDatos;

uint32_t ADC_val;
float temperature;

volatile uint32_t tiempo_tick;

extern TIM_HandleTypeDef htim3;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
		interrupt(GPIO_Pin, &htim3);

		//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
		//HAL_NVIC_EnableIRQ(EXTI0_IRQn);
		//HAL_NVIC_EnableIRQ(EXTI1_IRQn);
		//HAL_NVIC_EnableIRQ(EXTI2_IRQn);
		//HAL_NVIC_EnableIRQ(EXTI3_IRQn);

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	procesadoTemporizador(htim);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM3)
	{
		flag_tiempoTrans = 1; //Pasados los 3 segundos tras llamar a la planta, se deja de enviar
		HAL_TIM_Base_Stop_IT(&htim3);


	}

}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
		if(hadc-> Instance == ADC1 )
		{
			ADC_val = HAL_ADC_GetValue(&hadc1);
			temperature = getTemperature(ADC_val);
		}

	}





/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_SPI3_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);

  //HCSR04_Init();
  lcd_init();


  //for (volatile uint32_t i = 0; i < tres_s; i++)lcd_enviar("Selecciona:",0,1); // Retardo para evitar HAL_Delay()
  //lcd_clear();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  //Gestión sensor temperatura
	  gestorTemperatura();

	  //Gestión de la interrupción. Obtención de tecla pulsada
	  flagTecla(&key);

	  //Obtención posición del ascensor con ultrasonidos
	  //distancia = HCSR04_Get_Distance();
	  HCSR04_Read();
	  HAL_Delay(200);
	  distancia = getDistance();

	  /*sprintf(buf_lcd, "%lu", distancia);
	  lcd_clear();
	  lcd_enviar(buf_lcd, 0, 0); //(ms,row,colum-> mueve a la derecha) Centrado
	  lcd_send_string("     cm");
	  HAL_Delay(400);*/ //No se va a representar pero es el código usado para ver las distancias con el ultrasonidos

	  //Codificación de planta para envío
	  //posicion = calculaPosicion(distancia);
	  posicion = calculaPosicion(distancia);

	  //Codificación de tecla pulsada para envío
	  destino = calculaDestino(&key, &flag_tiempoTrans);

	  //Almacenamiento de la información a transmitir
	  actualizaInfoSPI();

	  //Envío de datos a través del SPI
	  enviaSPI();

	  //if(flag_tiempoTrans == 1) HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);

	  // Mostrar en el panel LCD la tecla pulsada durante 2s
	  //representaPlanta(key);//Comprobar que para 2s ha sucedido la transmisión y no hay una sobreescritura indeseada de key

	  //Funciones por si acaso
	  //lcd_barrido("Planta 2");
	  /*lcd_enviar("Planta 2", 0, 4); //(ms,row,colum-> mueve a la derecha) Centrado
	  HAL_Delay(5000);
	  lcd_clear();*/


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV16;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch) {
 ITM_SendChar(ch);
 return ch;
 }

void actualizaInfoSPI (void)
{
	if (cicloEnvio == 0)
		  {
			  envioDatos = posicion; //Ciclo primero envía el dato de la planta en la que se encuentra el ascensor
			  cicloEnvio++;
		  }
		  else
		  {
			  envioDatos = destino; //Ciclo segundo envía el dato del destino seleccionado
			  cicloEnvio = 0;
		  }
}

void enviaSPI (void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

	HAL_SPI_Transmit(&hspi3, &envioDatos, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);

}

void gestorTemperatura (void)
{
		  //Activación del ADC
		  HAL_ADC_Start(&hadc1);

		  //Obtener temperatura
		  if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK)
		  {
			  ADC_val = HAL_ADC_GetValue(&hadc1);
			  temperature = getTemperature(ADC_val);
		  }

		  //Desactivación del ADC
		  HAL_ADC_Stop(&hadc1);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
