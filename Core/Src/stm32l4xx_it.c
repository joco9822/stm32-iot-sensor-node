/*
 * stm32l4xx_it.c (발췌)
 * Wi-Fi 모듈의 CMD/DATA READY 핀(PE1, EXTI1)이 올라가면 flagData를 세운다.
 * wifi.c는 이 플래그로 Command Phase와 Data Phase의 시작을 기다린다.
 */
extern int flagData;

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(ISM43362_DRDY_EXTI1_Pin);
    flagData = 1;
}
