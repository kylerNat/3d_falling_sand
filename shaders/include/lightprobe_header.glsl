#define lightprobes_per_axis 16
#define lightprobe_spacing 32
#define lightprobes_w 64
#define lightprobes_h 64

#define lightprobe_resolution 6
#define lightprobe_padded_resolution (lightprobe_resolution+2)
#define lightprobe_resolution_x (lightprobes_w*lightprobe_padded_resolution)
#define lightprobe_resolution_y (lightprobes_h*lightprobe_padded_resolution)

#define lightprobe_depth_resolution 16
#define lightprobe_depth_padded_resolution (lightprobe_depth_resolution+2)
#define lightprobe_depth_resolution_x (lightprobes_w*lightprobe_depth_padded_resolution)
#define lightprobe_depth_resolution_y (lightprobes_h*lightprobe_depth_padded_resolution)

#define rays_per_lightprobe 4
