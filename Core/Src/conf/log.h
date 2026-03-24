/*
 * log.h
 *
 *  Created on: 2024年8月25日
 *      Author: Huang
 */

#ifndef SRC_CONF_LOG_H_
#define SRC_CONF_LOG_H_


#ifdef DEBUG
#define LOG_PRINTF(...) do { \
                            printf(__VA_ARGS__); \
                            printf("\n"); \
						} while (0)
#else
#define LOG_PRINTF(...) do {\
						}while(0)

#endif

#endif /* SRC_CONF_LOG_H_ */
