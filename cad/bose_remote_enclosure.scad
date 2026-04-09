$fn = 48;

part = "assembly"; // body, rear_lid, panel_layout, assembly
show_reference_hardware = true;

// Main envelope sized for OLED + encoder + status LED only.
outer_width = 92;
outer_depth = 76;
outer_height = 54;
corner_radius = 6.5;
wall = 2.4;
floor_thickness = 2.8;
front_panel_thickness = 3.0;
rear_lid_thickness = 3.0;

// Front panel layout: vertically centered stack.
oled_center = [outer_width / 2, 39.0];
oled_window = [24, 13];
oled_board = [27.4, 27.8];
oled_hole_spacing = [23.2, 23.2];
oled_mount_hole_d = 2.2;
oled_standoff_d = 5.8;
oled_backoff_from_panel = 5.2;

encoder_center = [outer_width / 2, 22.0];
encoder_panel_hole_d = 7.4;
encoder_relief_d = 15;
encoder_relief_depth = 1.2;

led_center = [outer_width / 2, 8.5];
led_hole_d = 2.2;
led_relief_d = 6.8;
led_relief_depth = 1.6;

// Rear lid and USB cable routing
rear_frame_depth = 8.0;
rear_frame_side_w = 12.0;
rear_opening_height = outer_height - wall;
rear_screw_hole_d = 3.2;
rear_screw_head_d = 6.4;
rear_screw_head_depth = 1.8;
rear_screw_x = 6.4;
rear_screw_z = [12, 42];
usb_opening = [13.0, 8.0];
usb_cable_slot_w = 8.0;
usb_slot_center_z = 8.8;
rear_vent = [7.0, 12.0];
rear_vent_x_offset = 20;

// S2 Mini tray
s2_board = [25.6, 34.6, 1.6]; // x, y, z; USB at rear on +Y edge
s2_floor_z = 5.0;
s2_rear_gap = 1.6;
s2_rail_width = 2.8;
s2_rail_capture = 1.2;
s2_entry_clearance = 0.6;

// Cable routing fences
cable_fence_thickness = 1.8;
cable_fence_height = 11;
cable_lane_x = 11;
cable_lane_w = 10;

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
  rotate([-90, 0, 0])
    linear_extrude(height = size[1])
      rounded_rect_2d([size[0], size[2]], radius);
}

function body_inner_radius() = max(corner_radius - wall, 1.5);
function rear_screw_positions() = [
  [rear_screw_x, rear_screw_z[0]],
  [rear_screw_x, rear_screw_z[1]],
  [outer_width - rear_screw_x, rear_screw_z[0]],
  [outer_width - rear_screw_x, rear_screw_z[1]]
];

function s2_origin() = [
  outer_width / 2 - s2_board[0] / 2,
  outer_depth - rear_lid_thickness - s2_rear_gap - s2_board[1],
  s2_floor_z
];

module front_panel_cutouts_2d() {
  translate([
    oled_center[0] - oled_window[0] / 2,
    oled_center[1] - oled_window[1] / 2
  ])
    rounded_rect_2d(oled_window, 1.8);

  translate([encoder_center[0], encoder_center[1]])
    circle(d = encoder_panel_hole_d);

  translate([led_center[0], led_center[1]])
    circle(d = led_hole_d);
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
      front_panel_thickness,
      oled_center[1] + offset[1]
    ]) {
      rotate([-90, 0, 0]) {
        difference() {
          cylinder(h = oled_backoff_from_panel, d = oled_standoff_d);
          translate([0, 0, -0.2])
            cylinder(h = oled_backoff_from_panel + 0.4, d = oled_mount_hole_d);
        }
      }
    }
  }
}

module front_panel_reliefs() {
  translate([encoder_center[0], front_panel_thickness - encoder_relief_depth, encoder_center[1]])
    rotate([-90, 0, 0])
      cylinder(h = encoder_relief_depth + 0.1, d = encoder_relief_d);

  translate([led_center[0], front_panel_thickness - led_relief_depth, led_center[1]])
    rotate([-90, 0, 0])
      cylinder(h = led_relief_depth + 0.1, d = led_relief_d);
}

module rear_frame_side_rails() {
  translate([0, outer_depth - rear_frame_depth, 0])
    cube([rear_frame_side_w, rear_frame_depth, outer_height]);
  translate([outer_width - rear_frame_side_w, outer_depth - rear_frame_depth, 0])
    cube([rear_frame_side_w, rear_frame_depth, outer_height]);
}

module rear_opening() {
  translate([
    rear_frame_side_w,
    outer_depth - rear_frame_depth - 0.1,
    0
  ])
    cube([
      outer_width - 2 * rear_frame_side_w,
      rear_frame_depth + 0.2,
      rear_opening_height
    ]);
}

module rear_screw_pilots() {
  for (pos = rear_screw_positions()) {
    translate([pos[0], outer_depth - rear_frame_depth - 0.1, pos[1]])
      rotate([90, 0, 0])
        cylinder(h = rear_frame_depth + 0.2, d = 2.6);
  }
}

module s2_tray() {
  origin = s2_origin();
  rail_len = s2_board[1] - 4;

  for (x_pos = [origin[0], origin[0] + s2_board[0] + s2_entry_clearance - s2_rail_width]) {
    translate([x_pos, origin[1] + 2, floor_thickness])
      cube([s2_rail_width, rail_len, s2_floor_z - floor_thickness]);
    translate([x_pos, origin[1] + 2, s2_floor_z])
      cube([s2_rail_width, rail_len, s2_rail_capture]);
  }

  translate([origin[0], origin[1], floor_thickness])
    cube([s2_board[0] + s2_entry_clearance, 2.0, s2_floor_z + s2_rail_capture - floor_thickness]);
}

module cable_guides() {
  translate([cable_lane_x, 14, floor_thickness])
    cube([cable_fence_thickness, outer_depth - 24, cable_fence_height]);
  translate([cable_lane_x + cable_lane_w, 24, floor_thickness])
    cube([cable_fence_thickness, outer_depth - 34, cable_fence_height]);
}

module side_reliefs() {
  for (x_pos = [0, outer_width - wall - 0.1]) {
    translate([x_pos, outer_depth - 24, 26])
      rotate([0, 90, 0])
        linear_extrude(height = wall + 0.2)
          rounded_rect_2d([16, 5], 1.8);
  }
}

module body() {
  difference() {
    union() {
      rounded_box_xz([outer_width, front_panel_thickness, outer_height], corner_radius);
      rounded_box_xz([outer_width, outer_depth, floor_thickness], corner_radius);
      translate([0, 0, 0]) cube([wall, outer_depth, outer_height]);
      translate([outer_width - wall, 0, 0]) cube([wall, outer_depth, outer_height]);
      translate([0, front_panel_thickness, outer_height - wall])
        cube([outer_width, outer_depth - front_panel_thickness, wall]);
      translate([0, outer_depth - rear_frame_depth, 0])
        cube([rear_frame_side_w, rear_frame_depth, outer_height]);
      translate([outer_width - rear_frame_side_w, outer_depth - rear_frame_depth, 0])
        cube([rear_frame_side_w, rear_frame_depth, outer_height]);

      oled_standoffs();
      s2_tray();
      cable_guides();
    }

    translate([0, -0.1, 0])
      rotate([-90, 0, 0])
        linear_extrude(height = front_panel_thickness + 0.2)
          front_panel_cutouts_2d();

    front_panel_reliefs();
    rear_screw_pilots();
    rear_opening();
    side_reliefs();
  }
}

module rear_lid() {
  difference() {
    rounded_box_xz([outer_width, rear_lid_thickness, outer_height], corner_radius);

    for (pos = rear_screw_positions()) {
      translate([pos[0], -0.1, pos[1]])
        rotate([-90, 0, 0])
          cylinder(h = rear_lid_thickness + 0.2, d = rear_screw_hole_d);

      translate([pos[0], rear_screw_head_depth, pos[1]])
        rotate([-90, 0, 0])
          cylinder(h = rear_screw_head_depth + 0.2, d = rear_screw_head_d);
    }

    translate([
      outer_width / 2 - usb_opening[0] / 2,
      -0.1,
      usb_slot_center_z - usb_opening[1] / 2
    ])
      rotate([-90, 0, 0])
        linear_extrude(height = rear_lid_thickness + 0.2)
          rounded_rect_2d(usb_opening, 1.8);

    // Open-bottom slot so the lid can close around an already connected USB cable.
    translate([
      outer_width / 2 - usb_cable_slot_w / 2,
      -0.1,
      0
    ])
      cube([usb_cable_slot_w, rear_lid_thickness + 0.2, usb_slot_center_z]);

    for (x_offset = [-rear_vent_x_offset, rear_vent_x_offset]) {
      translate([
        outer_width / 2 + x_offset - rear_vent[0] / 2,
        -0.1,
        22
      ])
        rotate([-90, 0, 0])
          linear_extrude(height = rear_lid_thickness + 0.2)
            rounded_rect_2d(rear_vent, 1.4);
    }
  }
}

module panel_layout() {
  difference() {
    rounded_box_xz([outer_width, 1.2, outer_height], corner_radius);

    translate([0, -0.1, 0])
      rotate([-90, 0, 0])
        linear_extrude(height = 1.4)
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
    translate(s2_origin())
      cube([s2_board[0], s2_board[1], s2_board[2]]);

  color([0.75, 0.75, 0.75, 0.5])
    translate([encoder_center[0], front_panel_thickness, encoder_center[1]])
      rotate([-90, 0, 0])
        cylinder(h = 18, d = 17);

  color([0.95, 0.2, 0.2, 0.45])
    translate([led_center[0], front_panel_thickness, led_center[1]])
      rotate([-90, 0, 0])
        cylinder(h = 4, d = 4.8);
}

module assembly() {
  color([0.94, 0.94, 0.94]) body();
  color([0.85, 0.85, 0.85])
    translate([0, outer_depth - rear_lid_thickness, 0])
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
