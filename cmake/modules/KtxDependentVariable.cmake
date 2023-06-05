#[============================================================================
# Copyright 2022, Khronos Group, Inc.
# SPDX-License-Identifier: Apache-2.0
#============================================================================]

#[=======================================================================[.rst:
KtxDependentVariable
--------------------

Macro to provide a cache variable dependent on other options.

This macro presents the variable to the user only if a set of other
conditions are true.

.. command:: dependent_variable

  .. code-block:: cmake

    KTX_DEPENDENT_VARIABLE(<var> type "<help_text>" <value> <depends> <force>)

  Makes ``<var>`` available to the user if the
  :ref:`semicolon-separated list <CMake Language Lists>` of conditions in
  ``<depends>`` are all true.  Otherwise, a local variable named ``<var>``
  is set to ``<force>``.

  When ``<var>`` is available, the given ``<help_text>`` and initial
  ``<value>`` are used. Otherwise, any value set by the user is preserved for
  when ``<depends>`` is satisfied in the future.

  Note that the ``<var>`` variable only has a value which satisfies the
  ``<depends>`` condition within the scope of the caller because it is a local
  variable.

Example invocation:

.. code-block:: cmake

  KTX_DEPENDENT_VARIABLE(USE_FOO STRING "Use Foo" "Default" "USE_BAR;NOT USE_ZOT" "")

If ``USE_BAR`` is true and ``USE_ZOT`` is false, this provides an var called
``USE_FOO`` that defaults to Default. Otherwise, it sets ``USE_FOO`` to OFF and
hides the var from the user. If the status of ``USE_BAR`` or ``USE_ZOT``
ever changes, any value for the ``USE_FOO`` var is saved so that when the
var is re-enabled it retains its old value.

An important difference to CMakeDependentOption is that this does an early
out on false when processing the list of dependencies. This is so expressions
of dependencies based on a potentially unset variable do not fail when it
is unset.

#]=======================================================================]

macro(KTX_DEPENDENT_VARIABLE var type doc default depends force)
  if(${var}_ISSET MATCHES "^${var}_ISSET$")
    set(${var}_AVAILABLE 1)
    foreach(d ${depends})
      cmake_language(EVAL CODE "
        if (${d})
        else()
          set(${var}_AVAILABLE 0)
        endif()"
      )
      if(NOT ${${var}_AVAILABLE})
          break()
      endif()
    endforeach()
    if(${var}_AVAILABLE)
      set(${var} "${${var}}" CACHE ${type} "${doc}" FORCE)
    else()
      if(${var} MATCHES "^${var}$")
      else()
        set(${var} "${${var}}" CACHE INTERNAL "${doc}")
      endif()
      set(${var} ${force})
    endif()
  else()
    set(${var} "${${var}_ISSET}")
  endif()
endmacro()
