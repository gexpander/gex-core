//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_I2C_INTERNAL_H
#define GEX_F072_I2C_INTERNAL_H

#ifndef I2C_INTERNAL
#error bad include!
#endif

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    uint8_t remap;      //!< I2C remap option

    bool anf;    //!< Enable analog noise filter
    uint8_t dnf; //!< Enable digital noise filter (1-15 ... max spike width)
    uint8_t speed; //!< 0 - Standard, 1 - Fast, 2 - Fast+

    I2C_TypeDef *periph;

//    GPIO_TypeDef *port;
//    uint32_t ll_pin_scl;
//    uint32_t ll_pin_sda;
};

#endif //GEX_F072_I2C_INTERNAL_H
