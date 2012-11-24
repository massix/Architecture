/*
 * Log.h
 *
 *  Created on: 24/nov/2012
 *      Author: massi
 */

#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG

#define LOG_MSG(msg) {  \
	std::cout << __FILE__ << ":" << __LINE__ << " - " << std::string(msg) << std::endl; \
}

#else
#define LOG_MSG() {}
#endif



#endif /* LOG_H_ */
