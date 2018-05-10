# LithiumBMS
This project is 1-6S Li-Ion/Li-Pol Battery management system, with balancer, load switch
and advanced features as external monitoring/setting over UART and temperature measurement
Development of this board began for BUT (VUT Brno in Czech), on Robotics course.

It also includes configurator app, which can be found in [this repo](https://github.com/JiriS97/LithiumBMS-Configurator).

## Supported batteries
All Li-Ion and Li-Pol batteries of 1-6 serially connected cells

## Parameters
| Parameter                                              |     Value    | Unit |
|--------------------------------------------------------|:------------:|:----:|
| Dimmensions                                            |   65x40x21   |  mm  |
| Maximal number of cells                                |       6      |   -  |
| Minimal number of cells                                |       1      |   -  |
| Maximal voltage of entire accupack                     |      26      |   V  |
| Maximal voltage per single cell                        |       6      |   V  |
| Maximal pulse current with this configuration          |      30      |   A  |
| Maximal current of WAGO connector                      |      16      |   A  |
| Maximal current of MOSFETs                             |      100     |   A  |
| Balancing current                                      |    60 ÷ 80   |  mA  |
| Peak power consumption                                 |       5      |  mA  |
| Idle power consumption                                 |      120     |  uA  |
| Typical consumed current when measuring U, I, P        |       2      |  mA  |
| Typical consumed power when measuring voltage of cells |       4      |  uA  |
| Typical average power consumption                      |      200     |  uA  |
| Typical current drain from cell that is being balanced |      70      |  mA  |
| Typical power drain from cell that is being balanced   |      200     |  mW  |
| Operating temperature range                            |   -20 ÷ +80  |  °C  |
| Storing temperature range                              |  -20 ÷ +100  |  °C  |
| Internal resistance of saturated power MOSFETs         |      4,5     |  mΩ  |
| Total internal resistance when MOSFEts are saturated   |       9      |  mΩ  |
| Period of cell voltage measurement                     | configurable |  ms  |
| Period of voltage, power and current measurement       | configurable |  ms  |
| Time of microcontroller sleep state                    | configurable |  ms  |

Detailed description of supported AT commands and the board itself can be found in attached PDF documents in the [Docs folder](Docs),
schematic and PCB layout in the [PCB folder](PCB)

## License
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details