$fn = 56;

part = "assembly"; // body, rear_lid, panel_layout, assembly
show_reference_hardware = true;

// Low-profile landscape enclosure: OLED on the left, encoder knob on the right.
outer_width = 88;
outer_depth = 34;
outer_height = 38;
corner_radius = 5.0;
wall = 2.2;
bottom_thickness = 2.6;
front_panel_thickness = 3.0;
rear_lid_thickness = 2.8;
body_depth = outer_depth - rear_lid_thickness;

// Front panel layout, measured in X/Z from the lower-left front corner.
oled_center = [24.0, 22.0];
oled_window = [24.0, 13.0];
oled_board = [27.4, 27.8];
oled_hole_spacing = [23.2, 23.2];
oled_mount_hole_d = 2.2;
oled_standoff_d = 5.6;
oled_backoff_from_panel = 5.2;

encoder_center = [64.0, 20.0];
encoder_panel_hole_d = 7.4;
encoder_relief_d = 15.5;
encoder_relief_depth = 1.2;
encoder_body_d = 17.0;
encoder_body_depth = 18.0;
knob_d = 22.0;
knob_depth = 14.0;

// LOLIN/Wemos S2 Mini lying flat on a left-side tray, USB-C facing rear.
s2_board = [34.6, 25.6, 1.6]; // x, y, z
s2_origin = [7.0, 4.1, 5.0];
s2_tray_clearance = 0.5;
s2_tray_floor_h = 0.8;
s2_rail_width = 2.8;
s2_rail_capture = 1.1;
s2_front_stop_h = 4.0;
s2_front_stop_thickness = 2.0;

usb_opening = [13.0, 8.0];
usb_slot_center_x = s2_origin[0] + s2_board[0] / 2;
usb_slot_center_z = 8.7;
usb_cable_slot_w = 8.0;

// Rear lid screws.
rear_screw_hole_d = 3.0;
rear_screw_pilot_d = 2.1;
rear_screw_head_d = 6.2;
rear_screw_head_depth = 1.6;
rear_screw_boss_d = 8.0;
rear_screw_boss_depth = 8.5;
rear_screw_x = [6.0, outer_width - 6.0];
rear_screw_z = [6.0, outer_height - 6.0];

// Small rear vents, kept away from the S2 Mini antenna area.
rear_vent = [14.0, 3.0];
rear_vent_x = [57.0, 73.0];
rear_vent_z = [13.0, 20.0, 27.0];

module rounded_rect_2d(size, radius) {
  x = size[0];
  y = size[1];
  hull() {
    for (ix = [radius, x - radius]) {
      for (iy = [radius, y - radius]) {
        translate([ix, iy]) circle(r = radius);
      }
    }
  }
}

module rounded_box_xz(size, radius) {
  x = size[0];
  y = size[1];
  z = size[2];

  hull() {
    for (ix = [radius, x - radius]) {
      for (iz = [radius, z - radius]) {
        translate([ix, 0, iz])
          rotate([-90, 0, 0])
            cylinder(h = y, r = radius);
      }
    }
  }
}

function inner_radius() = max(corner_radius - wall, 1.2);

function rear_screw_positions() = [
  [rear_screw_x[0], rear_screw_z[0]],
  [rear_screw_x[0], rear_screw_z[1]],
  [rear_screw_x[1], rear_screw_z[0]],
  [rear_screw_x[1], rear_screw_z[1]]
];

module extrude_panel_y(height) {
  translate([0, height, 0])
    rotate([90, 0, 0])
      linear_extrude(height = height)
        children();
}

module front_panel_cutouts_2d() {
  translate([
    oled_center[0] - oled_window[0] / 2,
    oled_center[1] - oled_window[1] / 2
  ])
    rounded_rect_2d(oled_window, 1.7);

  translate(encoder_center)
    circle(d = encoder_panel_hole_d);
}

module oled_standoffs() {
  offsets = [
    [-oled_hole_spacing[0] / 2, -oled_hole_spacing[1] / 2],
    [ oled_hole_spacing[0] / 2, -oled_hole_spacing[1] / 2],
    [-oled_hole_spacing[0] / 2,  oled_hole_spacing[1] / 2],
    [ oled_hole_spacing[0] / 2,  oled_hole_spacing[1] / 2]
  ];

  for (offset = offsets) {
    translate([
      oled_center[0] + offset[0],
      front_panel_thickness - 0.15,
      oled_center[1] + offset[1]
    ])
      rotate([-90, 0, 0])
        difference() {
          cylinder(h = oled_backoff_from_panel + 0.15, d = oled_standoff_d);
          translate([0, 0, -0.2])
            cylinder(h = oled_backoff_from_panel + 0.6, d = oled_mount_hole_d);
        }
  }
}

module front_panel_reliefs() {
  translate([
    encoder_center[0],
    front_panel_thickness - encoder_relief_depth,
    encoder_center[1]
  ])
    rotate([-90, 0, 0])
      cylinder(h = encoder_relief_depth + 0.1, d = encoder_relief_d);

}

module rear_screw_bosses() {
  for (pos = rear_screw_positions()) {
    translate([pos[0], body_depth - rear_screw_boss_depth, pos[1]])
      rotate([-90, 0, 0])
        cylinder(h = rear_screw_boss_depth, d = rear_screw_boss_d);

    if (pos[0] < outer_width / 2) {
      translate([0, body_depth - rear_screw_boss_depth, pos[1] - 1.5])
        cube([pos[0], rear_screw_boss_depth, 3.0]);
    } else {
      translate([pos[0], body_depth - rear_screw_boss_depth, pos[1] - 1.5])
        cube([outer_width - pos[0], rear_screw_boss_depth, 3.0]);
    }

    if (pos[1] < outer_height / 2) {
      translate([pos[0] - 1.5, body_depth - rear_screw_boss_depth, 0])
        cube([3.0, rear_screw_boss_depth, pos[1]]);
    } else {
      translate([pos[0] - 1.5, body_depth - rear_screw_boss_depth, pos[1]])
        cube([3.0, rear_screw_boss_depth, outer_height - pos[1]]);
    }
  }
}

module rear_screw_pilots() {
  for (pos = rear_screw_positions()) {
    translate([pos[0], body_depth - rear_screw_boss_depth - 0.2, pos[1]])
      rotate([-90, 0, 0])
        cylinder(h = rear_screw_boss_depth + 0.5, d = rear_screw_pilot_d);
  }
}

module s2_cradle() {
  tray_size = [
    s2_board[0] + 2 * s2_tray_clearance,
    s2_board[1] + s2_tray_clearance
  ];
  tray_origin = [
    s2_origin[0] - s2_tray_clearance,
    s2_origin[1] - s2_tray_clearance
  ];
  rail_y = s2_origin[1] + s2_front_stop_thickness + 0.8;
  rail_len = s2_board[1] - s2_front_stop_thickness - 1.6;
  rail_base_h = s2_origin[2] - bottom_thickness;

  translate([tray_origin[0], tray_origin[1], bottom_thickness - 0.2])
    cube([tray_size[0], tray_size[1], s2_tray_floor_h + 0.2]);

  for (x_pos = [
    s2_origin[0] - s2_tray_clearance,
    s2_origin[0] + s2_board[0] + s2_tray_clearance - s2_rail_width
  ]) {
    translate([x_pos, rail_y, bottom_thickness - 0.2])
      cube([s2_rail_width, rail_len, rail_base_h + 0.2]);

    translate([x_pos, rail_y, s2_origin[2]])
      cube([s2_rail_width, rail_len, s2_rail_capture]);
  }

  // Front stop: the board slides in from the open rear and stops here.
  translate([
    s2_origin[0] - s2_tray_clearance,
    s2_origin[1] - s2_tray_clearance,
    bottom_thickness - 0.2
  ])
    cube([
      s2_board[0] + 2 * s2_tray_clearance,
      s2_front_stop_thickness,
      s2_front_stop_h + 0.2
    ]);
}

module cable_lane() {
  translate([
    usb_slot_center_x - usb_cable_slot_w / 2,
    body_depth - 5.5,
    bottom_thickness
  ])
    cube([usb_cable_slot_w, 5.7, usb_slot_center_z - bottom_thickness + 3.2]);
}

module body_shell() {
  difference() {
    rounded_box_xz([outer_width, body_depth, outer_height], corner_radius);

    translate([wall, front_panel_thickness, bottom_thickness])
      rounded_box_xz([
        outer_width - 2 * wall,
        body_depth - front_panel_thickness + 0.2,
        outer_height - bottom_thickness - wall
      ], inner_radius());
  }
}

module body() {
  difference() {
    union() {
      body_shell();
      oled_standoffs();
      rear_screw_bosses();
      s2_cradle();
    }

    translate([0, -0.2, 0])
      extrude_panel_y(front_panel_thickness + 0.4)
        front_panel_cutouts_2d();

    front_panel_reliefs();
    rear_screw_pilots();
  }
}

module rear_lid_cutouts_2d() {
  for (pos = rear_screw_positions()) {
    translate(pos)
      circle(d = rear_screw_hole_d);
  }

  translate([
    usb_slot_center_x - usb_opening[0] / 2,
    usb_slot_center_z - usb_opening[1] / 2
  ])
    rounded_rect_2d(usb_opening, 1.6);

  // Open-bottom cable slot for closing the lid around a connected USB cable.
  translate([
    usb_slot_center_x - usb_cable_slot_w / 2,
    0
  ])
    square([usb_cable_slot_w, usb_slot_center_z]);

  for (x_pos = rear_vent_x) {
    for (z_pos = rear_vent_z) {
      translate([x_pos - rear_vent[0] / 2, z_pos - rear_vent[1] / 2])
        rounded_rect_2d(rear_vent, 1.4);
    }
  }
}

module rear_lid_countersinks() {
  for (pos = rear_screw_positions()) {
    translate([pos[0], rear_lid_thickness - rear_screw_head_depth, pos[1]])
      rotate([-90, 0, 0])
        cylinder(h = rear_screw_head_depth + 0.2, d = rear_screw_head_d);
  }
}

module rear_lid() {
  difference() {
    rounded_box_xz([outer_width, rear_lid_thickness, outer_height], corner_radius);

    translate([0, -0.1, 0])
      extrude_panel_y(rear_lid_thickness + 0.2)
        rear_lid_cutouts_2d();

    rear_lid_countersinks();
  }
}

module panel_layout() {
  difference() {
    rounded_box_xz([outer_width, 1.2, outer_height], corner_radius);

    translate([0, -0.1, 0])
      extrude_panel_y(1.4)
        front_panel_cutouts_2d();
  }
}

module reference_hardware() {
  color([0.10, 0.45, 0.85, 0.55])
    translate([
      oled_center[0] - oled_board[0] / 2,
      front_panel_thickness + oled_backoff_from_panel - 1.6,
      oled_center[1] - oled_board[1] / 2
    ])
      cube([oled_board[0], 1.6, oled_board[1]]);

  color([0.20, 0.75, 0.30, 0.55])
    translate(s2_origin)
      cube(s2_board);

  color([0.75, 0.75, 0.75, 0.5])
    translate([encoder_center[0], front_panel_thickness, encoder_center[1]])
      rotate([-90, 0, 0])
        cylinder(h = encoder_body_depth, d = encoder_body_d);

  color([0.08, 0.08, 0.08, 0.65])
    translate([encoder_center[0], -knob_depth, encoder_center[1]])
      rotate([-90, 0, 0])
        cylinder(h = knob_depth, d = knob_d);
}

module assembly() {
  color([0.94, 0.94, 0.94]) body();
  color([0.84, 0.84, 0.84])
    translate([0, body_depth, 0])
      rear_lid();

  if (show_reference_hardware) {
    reference_hardware();
  }
}

if (part == "body") {
  body();
} else if (part == "rear_lid") {
  rear_lid();
} else if (part == "panel_layout") {
  panel_layout();
} else {
  assembly();
}
