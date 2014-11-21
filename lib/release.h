
/****************************************************************************
 ** release.h ***************************************************************
 ****************************************************************************
 * Copyright (C) 2007 Christoph Bartelmus <lirc@bartelmus.de>
 */

/**
 * @file release.h
 * @brief Automatic release event generation.
 * @author Christoph Bartelmus
 * @ingroup private_api
 */

#ifndef RELEASE_H
#define RELEASE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "ir_remote_types.h"

void register_input(void);

void register_button_press(struct ir_remote* remote,
                           struct ir_ncode* ncode,
			   ir_code code, int reps);

void get_release_data(const char** remote_name,
		      const char** button_name,
		      int* reps);

void set_release_suffix(const char* s);

void get_release_time(struct timeval* tv);

const char* check_release_event(const char** remote_name,
				const char** button_name);

const char* trigger_release_event(const char** remote_name,
			      	  const char** button_name);

const char* release_map_remotes(struct ir_remote* old,
				struct ir_remote* new_remote,
				const char** remote_name,
				const char** button_name);


#ifdef	__cplusplus
}
#endif

#endif /* RELEASE_H */
