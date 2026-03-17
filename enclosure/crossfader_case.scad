// USB MIDI Crossfader — Enclosure
// Two-piece snap-fit case for Mini Innofader Plus + Teensy 4.0
// Print both pieces flat (no supports needed).
//
// Units: mm

// --- Dimensions (adjust to match your specific Innofader Plus) ---
fader_slot_length = 49;       // fader knob travel opening
fader_slot_width  = 5;        // width of the slot for the knob
fader_mount_spacing = 65;     // screw hole center-to-center (~1.5")
fader_mount_hole_dia = 3.2;   // M3 screw clearance
fader_body_length = 73;       // fader PCB length (along travel axis)
fader_body_width  = 16;       // fader PCB width

// Teensy 4.0 dimensions (without pins — soldering wires directly)
teensy_length = 36.3;
teensy_width  = 17.4;
teensy_height = 4;            // no pins, board + components only
usb_width     = 8;            // micro-USB port opening
usb_height    = 3.0;            // micro-USB port height

// Teensy cradle parameters
teensy_cradle_tolerance = 0.3;      // gap per side for friction fit
teensy_ledge_height = 2.5;          // height of ledge where PCB rests (SMD clearance below)
teensy_rail_total_height = 4.5;     // total rail height (extends above ledge to grip board)
teensy_rail_thickness = 1.5;        // outer rail wall thickness
teensy_ledge_width = 1.0;           // inward lip width that supports PCB edges
teensy_backstop_thickness = 1.5;    // end stop wall thickness

// Case parameters
wall = 2;
case_length = fader_body_length + teensy_length + wall * 3 + 5; // fader + gap + teensy
case_width  = max(fader_body_width, teensy_width) + wall * 2 + 6;
case_height = 18;             // raised for component clearance
corner_r    = 3;
fit_gap     = 3.0;            // base larger than top plate (1.5mm per side)
lid_clearance = 0.2;          // clearance around lid in rebate (0.1mm per side)
lower_wall = wall + 1.0;      // thicker walls below rebate to create wider ledge (3mm)

// Teensy area position (shared between base and top_plate)
teensy_area_start = wall + 2 + fader_body_length + wall + 5;

// LED hole (for WS2812B — light pipe or flush mount)
led_hole_dia = 5;

// Rubber foot recesses
foot_dia    = 10;
foot_depth  = 1;
foot_inset  = 8; // from corner

// --- Modules ---

module rounded_box(l, w, h, r) {
    hull() {
        for (x = [r, l-r])
            for (y = [r, w-r])
                translate([x, y, 0])
                    cylinder(h=h, r=r, $fn=30);
    }
}

// (snap-fit removed — lid friction-fits into rebate)

// --- Base (full-height box with rebate for lid) ---
module base() {
    off = fit_gap / 2;  // base extends this far beyond top plate on each side
    base_length = case_length + fit_gap;
    base_width  = case_width + fit_gap;

    difference() {
        // Outer shell — full case height
        translate([-off, -off, 0])
            rounded_box(base_length, base_width, case_height, corner_r);

        // Lower cavity (component area — thicker walls for ledge)
        translate([-off + lower_wall, -off + lower_wall, wall])
            rounded_box(base_length - lower_wall*2, base_width - lower_wall*2,
                        case_height, corner_r - 1);

        // Upper rebate (lid drops in here — thinner rim walls)
        translate([-lid_clearance/2, -lid_clearance/2, case_height / 2])
            rounded_box(case_length + lid_clearance, case_width + lid_clearance,
                        case_height, corner_r - 0.5);

        // USB port cutout (through base back wall — aligned to Teensy on ledge)
        usb_z = wall + teensy_ledge_height + 1.2;
        translate([case_length - wall - 1,
                   case_width/2 - usb_width/2,
                   usb_z])
            cube([wall + off + 2, usb_width, usb_height]);

        // Rubber foot recesses (bottom, centered on base)
        for (x = [foot_inset - off, base_length - foot_inset - off])
            for (y = [foot_inset - off, base_width - foot_inset - off])
                translate([x, y, -0.01])
                    cylinder(d=foot_dia, h=foot_depth, $fn=30);
    }

    // Teensy cradle — L-shaped rails with ledges, open underneath for SMDs
    teensy_cavity_width = teensy_width + teensy_cradle_tolerance * 2;
    teensy_center_y = case_width / 2;
    teensy_rail_y_left  = teensy_center_y - teensy_cavity_width / 2 - teensy_rail_thickness;
    teensy_rail_y_right = teensy_center_y + teensy_cavity_width / 2;
    teensy_rail_len = teensy_length - 6;  // shortened on USB end for clearance

    // Left rail wall (full height)
    translate([teensy_area_start, teensy_rail_y_left, wall])
        cube([teensy_rail_len, teensy_rail_thickness, teensy_rail_total_height]);

    // Left rail ledge (inward lip — PCB edge rests here)
    translate([teensy_area_start, teensy_rail_y_left + teensy_rail_thickness, wall])
        cube([teensy_rail_len, teensy_ledge_width, teensy_ledge_height]);

    // Right rail wall (full height)
    translate([teensy_area_start, teensy_rail_y_right, wall])
        cube([teensy_rail_len, teensy_rail_thickness, teensy_rail_total_height]);

    // Right rail ledge (inward lip — PCB edge rests here)
    translate([teensy_area_start, teensy_rail_y_right - teensy_ledge_width, wall])
        cube([teensy_rail_len, teensy_ledge_width, teensy_ledge_height]);

    // Backstop (non-USB end, prevents sliding toward fader)
    translate([teensy_area_start - teensy_backstop_thickness,
               teensy_rail_y_left,
               wall])
        cube([teensy_backstop_thickness,
              teensy_cavity_width + teensy_rail_thickness * 2,
              teensy_rail_total_height]);
}

// --- Top plate (lid — drops into base rebate) ---
module top_plate() {
    fader_area_start = wall + 2;

    difference() {
        // Lid outer shell
        rounded_box(case_length, case_width, case_height / 2, corner_r);

        // Hollow out underside (fader body hangs down into this cavity)
        translate([wall, wall, -0.01])
            rounded_box(case_length - wall*2, case_width - wall*2,
                        case_height / 2 - wall + 0.01, corner_r - 1);

        // Fader slot (knob travel)
        translate([fader_area_start + (fader_body_length - fader_slot_length) / 2,
                   case_width/2 - fader_slot_width/2,
                   -0.5])
            cube([fader_slot_length, fader_slot_width, case_height]);

        // Fader mounting holes (M3 clearance through 2mm ceiling)
        fader_center_x = fader_area_start + fader_body_length / 2;
        for (dx = [-fader_mount_spacing/2, fader_mount_spacing/2])
            translate([fader_center_x + dx, case_width/2, -0.5])
                cylinder(d=fader_mount_hole_dia, h=case_height, $fn=30);

        // LED hole (near the USB end)
        translate([case_length - wall - 12, case_width/2, -0.5])
            cylinder(d=led_hole_dia, h=case_height, $fn=30);
    }
}

// --- Render ---
// Base
base();

// Top plate / lid (offset for printing — move next to base)
translate([case_length + fit_gap + 10, 0, 0])
    top_plate();
