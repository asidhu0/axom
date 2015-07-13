// wrapDataView.h
// This is generated code, do not edit
//
// Copyright (c) 2015, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// All rights reserved.
//
// This source code cannot be distributed without permission and
// further review from Lawrence Livermore National Laboratory.
//
// wrapDataView.h
// For C users and C++ implementation

#ifndef WRAPDATAVIEW_H
#define WRAPDATAVIEW_H

#include "sidre/SidreTypes.h"
#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// declaration of wrapped types
#ifdef EXAMPLE_WRAPPER_IMPL
typedef void ATK_databuffer;
typedef void ATK_datagroup;
typedef void ATK_dataview;
#else
struct s_ATK_databuffer;
typedef struct s_ATK_databuffer ATK_databuffer;
struct s_ATK_datagroup;
typedef struct s_ATK_datagroup ATK_datagroup;
struct s_ATK_dataview;
typedef struct s_ATK_dataview ATK_dataview;
#endif

// splicer begin class.DataView.C_definition
// splicer end class.DataView.C_definition

void ATK_dataview_declare(ATK_dataview * self, int type, ATK_SidreLength len);

void ATK_dataview_allocate(ATK_dataview * self, int type, ATK_SidreLength len);

void ATK_dataview_reallocate(ATK_dataview * self, int type, ATK_SidreLength len);

bool ATK_dataview_has_buffer(ATK_dataview * self);

bool ATK_dataview_is_opaque(ATK_dataview * self);

const char * ATK_dataview_get_name(const ATK_dataview * self);

void * ATK_dataview_get_opaque(ATK_dataview * self);

ATK_databuffer * ATK_dataview_get_buffer(ATK_dataview * self);

void * ATK_dataview_get_data_pointer(ATK_dataview * self);

ATK_datagroup * ATK_dataview_get_owning_group(ATK_dataview * self);

int ATK_dataview_get_type_id(ATK_dataview * self);

size_t ATK_dataview_get_total_bytes(ATK_dataview * self);

size_t ATK_dataview_get_number_of_elements(ATK_dataview * self);

void ATK_dataview_print(ATK_dataview * self);

void ATK_dataview_set_value_int(ATK_dataview * self, int value);

void ATK_dataview_set_value_long(ATK_dataview * self, long value);

void ATK_dataview_set_value_float(ATK_dataview * self, float value);

void ATK_dataview_set_value_double(ATK_dataview * self, double value);

int ATK_dataview_get_value_int(ATK_dataview * self);

long ATK_dataview_get_value_long(ATK_dataview * self);

float ATK_dataview_get_value_float(ATK_dataview * self);

double ATK_dataview_get_value_double(ATK_dataview * self);

#ifdef __cplusplus
}
#endif

#endif  // WRAPDATAVIEW_H
