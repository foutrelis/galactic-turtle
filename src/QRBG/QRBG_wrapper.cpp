/* QRBG_wrapper.cpp - Wrapper for the QRBG C++ library. */

/* Copyright (C) 2008 Evangelos Foutras

   This file is part of Galactic Turtle.

   Galactic Turtle is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Galactic Turtle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Galactic Turtle.  If not, see <http://www.gnu.org/licenses/>. */

#define CUSTOM_CACHE_SIZE 1024*1024    /* We set QRBG's cache size to 1 MiB */

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <strings.h>
#include "QRBG.h"

static QRBG rnd_service(CUSTOM_CACHE_SIZE);

static int get_int() {
	try {
		return rnd_service.getInt();
	} catch (QRBG::ServiceDenied e) {
		std::cerr << "QRBG: " << e.why() << "." << endl;
		std::exit(1);
	}
}

extern "C" int QRBG_init() {
	char *pass;
	string user;
	
	std::cout << "QRBG username: ";
	std::cin >> user;
	pass = getpass("QRBG password: ");
	
	rnd_service.defineServer("random.irb.hr", 1227);
	rnd_service.defineUser(user.c_str(), pass);
	memset(pass, 0, sizeof(pass));
	
	return get_int();    /* small test to check if we can get random bytes */
}

extern "C" int QRBG_get_int() {
	return get_int();
}
