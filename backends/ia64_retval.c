/* Function return value location for IA64 ABI.
   Copyright (C) 2006 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <dwarf.h>

#define BACKEND ia64_
#include "libebl_CPU.h"


/* r8, or pair r8, r9, or aggregate up to r8-r11.  */
static const Dwarf_Op loc_intreg[] =
  {
    { .atom = DW_OP_reg8 }, { .atom = DW_OP_piece, .number = 8 },
    { .atom = DW_OP_reg9 }, { .atom = DW_OP_piece, .number = 8 },
    { .atom = DW_OP_reg10 }, { .atom = DW_OP_piece, .number = 8 },
    { .atom = DW_OP_reg11 }, { .atom = DW_OP_piece, .number = 8 },
  };
#define nloc_intreg	1
#define nloc_intregs(n)	(2 * (n))

/* f8, or aggregate up to f8-f15.  */
#define DEFINE_FPREG(size) 						      \
  static const Dwarf_Op loc_fpreg_##size[] =				      \
    {									      \
      { .atom = DW_OP_regx, .number = 128 + 8 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 9 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 10 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 11 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 12 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 13 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 14 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
      { .atom = DW_OP_regx, .number = 128 + 15 },			      \
      { .atom = DW_OP_piece, .number = size },				      \
    }
#define nloc_fpreg	1
#define nloc_fpregs(n)	(2 * (n))

DEFINE_FPREG (4);
DEFINE_FPREG (8);
DEFINE_FPREG (10);

#undef DEFINE_FPREG


/* The return value is a structure and is actually stored in stack space
   passed in a hidden argument by the caller.  But, the compiler
   helpfully returns the address of that space in r8.  */
static const Dwarf_Op loc_aggregate[] =
  {
    { .atom = DW_OP_breg8, .number = 0 }
  };
#define nloc_aggregate 1


int
ia64_return_value_location (Dwarf_Die *functypedie, const Dwarf_Op **locp)
{
  /* Start with the function's type, and get the DW_AT_type attribute,
     which is the type of the return value.  */

  Dwarf_Attribute attr_mem;
  Dwarf_Attribute *attr = dwarf_attr (functypedie, DW_AT_type, &attr_mem);
  if (attr == NULL)
    /* The function has no return value, like a `void' function in C.  */
    return 0;

  Dwarf_Die die_mem;
  Dwarf_Die *typedie = dwarf_formref_die (attr, &die_mem);
  int tag = dwarf_tag (typedie);

  /* Follow typedefs and qualifiers to get to the actual type.  */
  while (tag == DW_TAG_typedef
	 || tag == DW_TAG_const_type || tag == DW_TAG_volatile_type
	 || tag == DW_TAG_restrict_type || tag == DW_TAG_mutable_type)
    {
      attr = dwarf_attr (typedie, DW_AT_type, &attr_mem);
      typedie = dwarf_formref_die (attr, &die_mem);
      tag = dwarf_tag (typedie);
    }

  Dwarf_Word size;
  switch (tag)
    {
    case -1:
      return -1;

    case DW_TAG_subrange_type:
      if (! dwarf_hasattr (typedie, DW_AT_byte_size))
	{
	  attr = dwarf_attr (typedie, DW_AT_type, &attr_mem);
	  typedie = dwarf_formref_die (attr, &die_mem);
	  tag = dwarf_tag (typedie);
	}
      /* Fall through.  */

    case DW_TAG_base_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
      if (dwarf_formudata (dwarf_attr (typedie, DW_AT_byte_size,
				       &attr_mem), &size) != 0)
	{
	  if (tag == DW_TAG_pointer_type || tag == DW_TAG_ptr_to_member_type)
	    size = 8;
	  else
	    return -1;
	}
      if (tag == DW_TAG_base_type)
	{
	  Dwarf_Word encoding;
	  if (dwarf_formudata (dwarf_attr (typedie, DW_AT_encoding,
					   &attr_mem), &encoding) != 0)
	    return -1;

	  switch (encoding)
	    {
	    case DW_ATE_float:
	      switch (size)
		{
		case 4:		/* float */
		  *locp = loc_fpreg_4;
		  return nloc_fpreg;
		case 8:		/* double */
		  *locp = loc_fpreg_8;
		  return nloc_fpreg;
		case 10:       /* x86-style long double, not really used */
		  *locp = loc_fpreg_10;
		  return nloc_fpreg;
		case 16:	/* long double, IEEE quad format */
		  *locp = loc_intreg;
		  return nloc_intregs (2);
		}
	      return -2;

	    case DW_ATE_complex_float:
	      switch (size)
		{
		case 4 * 2:	/* complex float */
		  *locp = loc_fpreg_4;
		  return nloc_fpregs (2);
		case 8 * 2:	/* complex double */
		  *locp = loc_fpreg_8;
		  return nloc_fpregs (2);
		case 10 * 2:	/* complex long double (x86-style) */
		  *locp = loc_fpreg_10;
		  return nloc_fpregs (2);
		case 16 * 2:	/* complex long double (IEEE quad) */
		  *locp = loc_intreg;
		  return nloc_intregs (4);
		}
	      return -2;
	    }
	}

    intreg:
      *locp = loc_intreg;
      if (size <= 8)
	return nloc_intreg;
      if (size <= 32)
	return nloc_intregs ((size + 7) / 8);

    large:
      *locp = loc_aggregate;
      return nloc_aggregate;

    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
    case DW_TAG_array_type:
      if (dwarf_formudata (dwarf_attr (typedie, DW_AT_byte_size,
				       &attr_mem), &size) != 0)
	return -1;
      if (size > 32)
	goto large;

      /* XXX
	 Must examine the fields in picayune ways to determine the
	 actual answer.  If it qualifies as an HFA (all floats)
	 then it should be returned in fp regs.
      */

      goto intreg;
    }

  /* XXX We don't have a good way to return specific errors from ebl calls.
     This value means we do not understand the type, but it is well-formed
     DWARF and might be valid.  */
  return -2;
}