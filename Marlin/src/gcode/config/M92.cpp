/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../gcode.h"
#include "../../module/planner.h"

void report_M92(const bool echo=true, const int8_t e=-1) {
  if (echo) SERIAL_ECHO_START(); else SERIAL_CHAR(' ');
  SERIAL_ECHOPAIR_P(LIST_N(DOUBLE(LINEAR_AXES),
    PSTR(" M92 X"), LINEAR_UNIT(planner.settings.axis_steps_per_mm[X_AXIS]),
    SP_Y_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[Y_AXIS]),
    SP_Z_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[Z_AXIS]),
    SP_I_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[I_AXIS]),
    SP_J_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[J_AXIS]),
    SP_K_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[K_AXIS]),
    SP_M_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[M_AXIS]),
    SP_O_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[O_AXIS]),
    SP_P_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[P_AXIS]),
    SP_Q_STR, LINEAR_UNIT(planner.settings.axis_steps_per_mm[Q_AXIS]))
  );
  #if HAS_EXTRUDERS && DISABLED(DISTINCT_E_FACTORS)
    SERIAL_ECHOPAIR_P(SP_E_STR, VOLUMETRIC_UNIT(planner.settings.axis_steps_per_mm[E_AXIS]));
  #endif
  SERIAL_EOL();

  #if ENABLED(DISTINCT_E_FACTORS)
    LOOP_L_N(i, E_STEPPERS) {
      if (e >= 0 && i != e) continue;
      if (echo) SERIAL_ECHO_START(); else SERIAL_CHAR(' ');
      SERIAL_ECHOLNPAIR_P(PSTR(" M92 T"), i,
                        SP_E_STR, VOLUMETRIC_UNIT(planner.settings.axis_steps_per_mm[E_AXIS_N(i)]));
    }
  #endif

  UNUSED(e);
}

/**
 * M92: Set axis steps-per-unit for one or more axes, X, Y, Z, and E.
 *      (Follows the same syntax as G92)
 *
 *      With multiple extruders use T to specify which one.
 *
 *      If no argument is given print the current values.
 *
 *    With MAGIC_NUMBERS_GCODE:
 *      Use 'H' and/or 'L' to get ideal layer-height information.
 *      'H' specifies micro-steps to use. We guess if it's not supplied.
 *      'L' specifies a desired layer height. Nearest good heights are shown.
 */
void GcodeSuite::M92() {

  const int8_t target_extruder = get_target_extruder_from_command();
  if (target_extruder < 0) return;

  // No arguments? Show M92 report.
  if (!parser.seen(
    LOGICAL_AXIS_GANG("E", "X", "Y", "Z", AXIS4_STR, AXIS5_STR, AXIS6_STR, AXIS7_STR, AXIS8_STR, AXIS9_STR, AXIS10_STR)
    TERN_(MAGIC_NUMBERS_GCODE, "HL")
  )) return report_M92(true, target_extruder);

  LOOP_LOGICAL_AXES(i) {
    if (parser.seenval(axis_codes[i])) {
      if (TERN1(HAS_EXTRUDERS, i != E_AXIS))
        planner.settings.axis_steps_per_mm[i] = parser.value_per_axis_units((AxisEnum)i);
      else {
        #if HAS_EXTRUDERS
          const float value = parser.value_per_axis_units((AxisEnum)(E_AXIS_N(target_extruder)));
          if (value < 20) {
            float factor = planner.settings.axis_steps_per_mm[E_AXIS_N(target_extruder)] / value; // increase e constants if M92 E14 is given for netfab.
            #if HAS_CLASSIC_JERK && HAS_CLASSIC_E_JERK
              planner.max_jerk.e *= factor;
            #endif
            planner.settings.max_feedrate_mm_s[E_AXIS_N(target_extruder)] *= factor;
            planner.max_acceleration_steps_per_s2[E_AXIS_N(target_extruder)] *= factor;
          }
          planner.settings.axis_steps_per_mm[E_AXIS_N(target_extruder)] = value;
        #endif
      }
    }
  }
  planner.refresh_positioning();

  #if ENABLED(MAGIC_NUMBERS_GCODE)
    #ifndef Z_MICROSTEPS
      #define Z_MICROSTEPS 16
    #endif
    const float wanted = parser.floatval('L');
    if (parser.seen('H') || wanted) {
      const uint16_t argH = parser.ushortval('H'),
                     micro_steps = argH ?: Z_MICROSTEPS;
      const float z_full_step_mm = micro_steps * planner.steps_to_mm[Z_AXIS];
      SERIAL_ECHO_START();
      SERIAL_ECHOPAIR("{ micro_steps:", micro_steps, ", z_full_step_mm:", z_full_step_mm);
      if (wanted) {
        const float best = uint16_t(wanted / z_full_step_mm) * z_full_step_mm;
        SERIAL_ECHOPAIR(", best:[", best);
        if (best != wanted) { SERIAL_CHAR(','); SERIAL_DECIMAL(best + z_full_step_mm); }
        SERIAL_CHAR(']');
      }
      SERIAL_ECHOLNPGM(" }");
    }
  #endif
}
