// ===========================================================================
//  battery.h — Leitura da bateria (BAT_ADC no GPIO1) e status de carga.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace battery {
    void  begin();
    float voltage();    // tensão da bateria em volts
    int   percent();    // 0..100 (curva aproximada de LiPo)
    bool  charging();   // true se estiver carregando (CHG_STAT baixo)
}
