module EEE_IMGPROC(
	// global clock & reset
	clk,
	reset_n,
	
	// mm slave
	s_chipselect,
	s_read,
	s_write,
	s_readdata,
	s_writedata,
	s_address,

	// stream sink
	sink_data,
	sink_valid,
	sink_ready,
	sink_sop,
	sink_eop,
	
	// streaming source
	source_data,
	source_valid,
	source_ready,
	source_sop,
	source_eop,
	
	// conduit
	mode,
	message_to_ESP32,
	message_from_ESP32
	// outbuffer,
	// receive_msg
);


// global clock & reset
input	clk;
input	reset_n;

// mm slave
input							s_chipselect;
input							s_read;
input							s_write;
output	reg	[31:0]	s_readdata;
input	[31:0]				s_writedata;
input	[2:0]					s_address;

// streaming sink
input	[23:0]            	sink_data;
input								sink_valid;
output							sink_ready;
input								sink_sop;
input								sink_eop;

// streaming source
output	[23:0]			  	   source_data;
output								source_valid;
input									source_ready;
output								source_sop;
output								source_eop;

// conduit export
input                         mode;
output		reg		[15:0]    message_to_ESP32;
input				[15:0]    message_from_ESP32;


////////////////////////////////////////////////////////////////////////
//
//HSV and Luminance

parameter IMAGE_W = 11'd640;
parameter IMAGE_H = 11'd480;
parameter MESSAGE_BUF_MAX = 256;
parameter MSG_INTERVAL = 6;
parameter BB_COL_DEFAULT = 24'h00ff00;
parameter horizontal_edge_region_threshold = 6'd30;
parameter vertical_region_confirm_threshold = 6'd50;
parameter difference_threshold = 8'd150;
parameter count_threshold = 6'd10;
parameter y_threshold = 6'd20;


wire [7:0]   red, green, blue, grey;
wire [7:0]   red_out, green_out, blue_out;

wire         sop, eop, in_valid, out_ready;
////////////////////////////////////////////////////////////////////////
wire red_detected, green_detected, pink_detected, orange_detected, black_detected;
wire [23:0] color_high;

//reg [7:0] min_value;
//reg signed [9:0] hue; 

wire[9:0] hue ;
wire[7:0] saturation, value, min;

// reg [7:0] luminosity;
// reg [7:0] saturation;

reg [7:0] Red_stage_1, Red_stage_2, Red_stage_3, Red_stage_4, Red_stage_5;
reg [7:0] Green_stage_1, Green_stage_2, Green_stage_3, Green_stage_4, Green_stage_5;
reg [7:0] Blue_stage_1, Blue_stage_2, Blue_stage_3, Blue_stage_4, Blue_stage_5;


always @(posedge clk) begin
	Red_stage_1 <= red;
	Red_stage_2 <= Red_stage_1;
	Red_stage_3 <= Red_stage_2;
	Red_stage_4 <= Red_stage_3;
	Red_stage_5 <= Red_stage_4;
	Green_stage_1 <= green;
	Green_stage_2 <= Green_stage_1;
	Green_stage_3 <= Green_stage_2;
	Green_stage_4 <= Green_stage_3;
	Green_stage_5 <= Green_stage_4;
	Blue_stage_1 <= blue;
	Blue_stage_2 <= Blue_stage_1;
	Blue_stage_3 <= Blue_stage_2;
	Blue_stage_4 <= Blue_stage_3;
	Blue_stage_5 <= Blue_stage_4;
end
// [0.006 0.061 0.242 0.383 0.242 0.061 0.006]

// [0.061 0.242 0.383 0.242 0.061] 

// [8/128 , 31/128 , 49/128 , 31/128 , 8/128]
reg [14:0] tmp_r_1, tmp_r_2, tmp_r_3, tmp_r_4, tmp_r_5;
reg [14:0] tmp_g_1, tmp_g_2, tmp_g_3, tmp_g_4, tmp_g_5;
reg [14:0] tmp_b_1, tmp_b_2, tmp_b_3, tmp_b_4, tmp_b_5;
reg [14:0] tmp_sum_1, tmp_sum_2, tmp_sum_3, tmp_sum_4, tmp_sum_5;
reg [7:0] can_input11, can_input12, can_input13;
reg [7:0] can_input21, can_input22, can_input23;
reg [7:0] can_input31, can_input32, can_input33;


//////////////////////////////////////////////////////////
//Guassian Filter
//////////////////////////////////////////////////////////
reg [7:0] smooth_red, smooth_green, smooth_blue;
always @(*) begin

	if (x < 2) begin
		smooth_red = red;
		smooth_green = green;
		smooth_blue = blue;
	end 
	//11'h2
	if (x % IMAGE_W > IMAGE_W - 2 ) begin
		smooth_red = Red_stage_5;
		smooth_green = Green_stage_5;
		smooth_blue = Blue_stage_5;
	end 
	else begin

		tmp_r_1 = Red_stage_1 * 8; 
		tmp_r_2 = Red_stage_2 * 31;
		tmp_r_3 = Red_stage_3 * 49;
		tmp_r_4 = Red_stage_4 * 31;
		tmp_r_5 = Red_stage_5 * 8;
		tmp_sum_1 = tmp_r_1 + tmp_r_2 + tmp_r_3 + tmp_r_4 + tmp_r_5;
		smooth_red = tmp_sum_1 [14:7];
		//smooth_red = red;

		tmp_g_1 = Green_stage_1 * 8; 
		tmp_g_2 = Green_stage_2 * 31;
		tmp_g_3 = Green_stage_3 * 49;
		tmp_g_4 = Green_stage_4 * 31;
		tmp_g_5 = Green_stage_5 * 8;
		tmp_sum_2 = tmp_g_1 + tmp_g_2 + tmp_g_3 + tmp_g_4 + tmp_g_5;
		smooth_green = tmp_sum_2 [14:7];
		//smooth_green = green;

		tmp_b_1 = Blue_stage_1 * 8; 
		tmp_b_2 = Blue_stage_2 * 31;
		tmp_b_3 = Blue_stage_3 * 49;
		tmp_b_4 = Blue_stage_4 * 31;
		tmp_b_5 = Blue_stage_5 * 8;
		tmp_sum_3 = tmp_b_1 + tmp_b_2 + tmp_b_3 + tmp_b_4 + tmp_b_5;
		smooth_blue = tmp_sum_3 [14:7];
		//smooth_blue = blue;

	end 
end


//////////////////////////////////////////////////////////
//Median Filter
//////////////////////////////////////////////////////////
wire [7:0] median_red, median_green, median_blue;
reg [7:0] Red_median_stage_1, Red_median_stage_2, Red_median_stage_3, Red_median_stage_4, Red_median_stage_5;
reg [7:0] Green_median_stage_1, Green_median_stage_2, Green_median_stage_3, Green_median_stage_4, Green_median_stage_5;
reg [7:0] Blue_median_stage_1, Blue_median_stage_2, Blue_median_stage_3, Blue_median_stage_4, Blue_median_stage_5;
always @(posedge clk) begin
	Red_median_stage_1 <= smooth_red;
	Red_median_stage_2 <= Red_median_stage_1;
	Red_median_stage_3 <= Red_median_stage_1;
	Red_median_stage_4 <= Red_median_stage_3;
	Red_median_stage_5 <= Red_median_stage_4;
	Green_median_stage_1 <= smooth_green;
	Green_median_stage_2 <= Green_median_stage_1;
	Green_median_stage_3 <= Green_median_stage_2;
	Green_median_stage_4 <= Green_median_stage_3;
	Green_median_stage_5 <= Green_median_stage_5;
	Blue_median_stage_1 <= smooth_blue;
	Blue_median_stage_2 <= Blue_median_stage_1;
	Blue_median_stage_3 <= Blue_median_stage_2;
	Blue_median_stage_4 <= Blue_median_stage_3;
	Blue_median_stage_5 <= Blue_median_stage_5;
end

Median M_red( 
	.x_value(x),
	.smooth_value(smooth_red),
	.reg_5(Red_median_stage_5),
	.a(Red_median_stage_1),
	.b(Red_median_stage_2),
	.c(Red_median_stage_3),
	.d(Red_median_stage_4),
	.e(Red_median_stage_5),
	.median(median_red)
);

Median M_green( 
	.x_value(x),
	.smooth_value(smooth_green),
	.reg_5(Green_median_stage_5),
	.a(Green_stage_1),
	.b(Green_stage_2),
	.c(Green_stage_3),
	.d(Green_stage_4),
	.e(Green_stage_5),
	.median(median_green)
);

Median M_blue( 
	.x_value(x),
	.smooth_value(smooth_blue),
	.reg_5(Blue_median_stage_5),
	.a(Blue_stage_1),
	.b(Blue_stage_2),
	.c(Blue_stage_3),
	.d(Blue_stage_4),
	.e(Blue_stage_5),
	.median(median_blue)
);



///////////////////////////////////////////////////////////////////
//HSV Convertion
///////////////////////////////////////////////////////////////////
assign value = (red > green) ? ((red > blue) ? red[7:0] : blue[7:0]) : (green > blue) ? green[7:0] : blue[7:0];						
assign min = (red < green)? ((red<blue) ? red[7:0] : blue[7:0]) : (green < blue) ? green [7:0] : blue[7:0];
assign saturation = (value - min)* 255 / value;
assign hue = (red == green && red == blue) ? 0 :((value != red)? (value != green) ? (((240*((value - min))+ (60* (red - green)))/(value-min))>>1):
                ((120*(value-min)+60*(blue - red))/(value - min)>>1): 
                (blue < green) ? ((60*(green - blue)/(value - min))>>1): (((360*(value-min) +(60*(green - blue)))/(value - min))>>1));


reg red_detected_1,red_detected_2,red_detected_3 ,red_detected_4, red_detected_5, red_detected_6;
reg pink_detected_1,pink_detected_2,pink_detected_3 ,pink_detected_4, pink_detected_5, pink_detected_6;
reg green_detected_1,green_detected_2,green_detected_3, green_detected_4, green_detected_5, green_detected_6;
reg orange_detected_1, orange_detected_2, orange_detected_3, orange_detected_4, orange_detected_5, orange_detected_6;
reg black_detected_1, black_detected_2, black_detected_3, black_detected_4, black_detected_5, black_detected_6;
reg cyan_detected_1, cyan_detected_2, cyan_detected_3, cyan_detected_4, cyan_detected_5, cyan_detected_6;
reg yellow_detected_1, yellow_detected_2, yellow_detected_3, yellow_detected_4, yellow_detected_5, yellow_detected_6;
reg blue_detected_1, blue_detected_2, blue_detected_3, blue_detected_4, blue_detected_5, blue_detected_6;



initial begin
	red_detected_1 = 0;
	red_detected_2 = 0;
	red_detected_3 = 0;
	red_detected_4 = 0;
	red_detected_5 = 0;
	red_detected_6 = 0;
	
	pink_detected_1 = 0;
	pink_detected_2 = 0;
	pink_detected_3 = 0;
	pink_detected_4 = 0;
	pink_detected_5 = 0;
	pink_detected_6 = 0;

	green_detected_1 = 0;
	green_detected_2 = 0;
	green_detected_3 = 0;
	green_detected_4 = 0;
	green_detected_5 = 0;
	green_detected_6 = 0;

	orange_detected_1 = 0;
	orange_detected_2 = 0;
	orange_detected_3 = 0;
	orange_detected_4 = 0;
	orange_detected_5 = 0;
	orange_detected_6 = 0;
	
	black_detected_1 = 0;
	black_detected_2 = 0;
	black_detected_3 = 0;
	black_detected_4 = 0;
	black_detected_5 = 0;
	black_detected_6 = 0;

	cyan_detected_1 = 0;
	cyan_detected_2 = 0;
	cyan_detected_3 = 0;
	cyan_detected_4 = 0;
	cyan_detected_5 = 0;
	cyan_detected_6 = 0;

	yellow_detected_1 = 0;
	yellow_detected_2 = 0;
	yellow_detected_3 = 0;
	yellow_detected_4 = 0;
	yellow_detected_5 = 0;
	yellow_detected_6 = 0;

	blue_detected_1 = 0;
	blue_detected_2 = 0;
	blue_detected_3 = 0;
	blue_detected_4 = 0;
	blue_detected_5 = 0;
	blue_detected_6 = 0;
end


always @(posedge clk)begin
	red_detected_1 <= red_detected;
	red_detected_2 <= red_detected_1;
	red_detected_3 <= red_detected_2;
	red_detected_4 <= red_detected_3;
	red_detected_5 <= red_detected_4;
	red_detected_6 <= red_detected_5;
	
	pink_detected_1 <= pink_detected;
	pink_detected_2 <= pink_detected_1;
	pink_detected_3 <= pink_detected_2;
	pink_detected_4 <= pink_detected_3;
	pink_detected_5 <= pink_detected_4;
	pink_detected_6 <= pink_detected_5;
	
	green_detected_1 <= green_detected;
	green_detected_2 <= green_detected_1;
	green_detected_3 <= green_detected_2;
	green_detected_4 <= green_detected_3;
	green_detected_5 <= green_detected_4;
	green_detected_6 <= green_detected_5;
	
	orange_detected_1 <= orange_detected;
	orange_detected_2 <= orange_detected_1;
	orange_detected_3 <= orange_detected_2;
	orange_detected_4 <= orange_detected_3;
	orange_detected_5 <= orange_detected_4;
	orange_detected_6 <= orange_detected_5;

	black_detected_1 <= black_detected;
	black_detected_2 <= black_detected_1;
	black_detected_3 <= black_detected_2;
	black_detected_4 <= black_detected_3;
	black_detected_5 <= black_detected_4;
	black_detected_6 <= black_detected_5;

    cyan_detected_1 <= cyan_detected;
	cyan_detected_2 <= cyan_detected_1;
	cyan_detected_3 <= cyan_detected_2;
	cyan_detected_4 <= cyan_detected_3;
	cyan_detected_5 <= cyan_detected_4;
	cyan_detected_6 <= cyan_detected_5;

	yellow_detected_1 <= yellow_detected;
	yellow_detected_2 <= yellow_detected_1;
	yellow_detected_3 <= yellow_detected_2;
	yellow_detected_4 <= yellow_detected_3;
	yellow_detected_5 <= yellow_detected_4;
	yellow_detected_6 <= yellow_detected_5;

	blue_detected_1 <= blue_detected;
	blue_detected_2 <= blue_detected_1;
	blue_detected_3 <= blue_detected_2;
	blue_detected_4 <= blue_detected_3;
	blue_detected_5 <= blue_detected_4;
	blue_detected_6 <= blue_detected_5;
    
end


/////////////////////////////////////////////// HSV /////////////////////////////////////////////// 

assign pink_detected = 
(6 < hue && hue < 21) && (135 < saturation  && saturation < 220) && (170 < value);
// ( (150 < hue && hue < 180) &&  (90 < saturation && saturation < 120) && (200 < value))
// || ((hue < 15 && hue >= 10) && (saturation < 125 && saturation > 100) && (value > 140))
// || ((hue < 23 && hue >= 15) && (saturation < 100 && saturation > 70) && (value > 140))
// || ((hue < 10) && (saturation < 110 && saturation > 50) && (value > 80));

assign orange_detected = 
( ((hue >= 11 && hue <= 15) && (value > 155 && saturation > 150)) 
|| (( 15 < hue && hue < 20) && (saturation > 110) && (value > 125) ))
|| ((hue < 12 && hue >= 10) && ( saturation >= 125) && (value > 140));

// assign green_detected = ((hue >= 50 && hue <= 75) && (saturation > 105 && value >= 25 )) || ((hue >= 50 && hue <= 75) && ((saturation > 127 && value > 173)))
// ||  ((hue > 40 && hue < 90) && ( 10 < value && value < 80) && (saturation > 30));

assign green_detected = (44 < hue && hue < 62) && (144 < saturation && saturation < 204) && (value > 124); 
// (55 < hue && hue < 70) && (saturation > 153) && (value > 175)
// || (45 < hue <= 55) && (saturation > 125) && ();

assign red_detected = (hue <= 7 && saturation > 150 && value > 50) 
|| ((hue < 360 && hue > 330) && (saturation > 150) && value > 50)
|| ((hue < 12 && hue > 7) && (saturation > 170) && value > 170);	

assign cyan_detected = (23 < hue && hue < 80) && (28 < saturation && saturation < 179) && ( 44 < value && value < 70);

assign black_detected = (value < 25);

assign blue_detected = (hue > 78 && hue < 130) && ( 84 < saturation && saturation < 140);

assign yellow_detected = (26 < hue && hue < 31) && (123 < saturation && saturation < 219 ) && (value > 237);



/////////////////////////////////////////////// HSV /////////////////////////////////////////////// 

wire red_final_detected, pink_final_detected, green_final_detected, orange_final_detected, black_final_detected, cyan_final_detected, yellow_final_detected, blue_final_detected;

assign red_final_detected = red_detected_1 && red_detected_2 && red_detected_3 && red_detected_4 && red_detected_5 && red_detected_6;
assign pink_final_detected = pink_detected_1 && pink_detected_2 && pink_detected_3 && pink_detected_4 && pink_detected_5 && pink_detected_6;
assign green_final_detected = green_detected_1 && green_detected_2 && green_detected_3 && green_detected_4 && green_detected_5 && green_detected_6;
assign orange_final_detected = orange_detected_1 && orange_detected_2 && orange_detected_3 && orange_detected_4 && orange_detected_5 && orange_detected_6;
assign black_final_detected = black_detected_1 && black_detected_2 && black_detected_3 && black_detected_4 && black_detected_5 && black_detected_6;
assign cyan_final_detected = cyan_detected_1 && cyan_detected_2 && cyan_detected_3 && cyan_detected_4 && cyan_detected_5 && cyan_detected_6;
assign yellow_final_detected = yellow_detected_1 && yellow_detected_2 && yellow_detected_3 && yellow_detected_4 && yellow_detected_5 && yellow_detected_6;
assign blue_final_detected = blue_detected_1 && blue_detected_2 && blue_detected_3 && blue_detected_4 && blue_detected_5 && blue_detected_6;

assign grey = green[7:1] + red[7:2] + blue[7:2]; //Grey = green/2 + red/4 + blue/4

assign color_high  =  
					  (red_final_detected) ? {8'hff, 8'h0, 8'h0} : 
					  (green_final_detected) ? {24'h59e02c} :
					  (pink_final_detected) ? {8'hff,8'h00,8'h5d} :
					  (orange_final_detected) ? {8'hff,8'h77,8'h00} : 
					  (black_final_detected) ? (24'h00ffff) : 
					  (cyan_final_detected) ? (24'h2fbd9f):
					  (yellow_final_detected) ? (24'hede26f):
					  (blue_final_detected) ? (24'h51e5f4):
					  {grey, grey, grey};

//red_high pure red if red_detect else grey.


// Show bounding box

wire [23:0] new_image;
wire bb_active_r, bb_active_g, bb_active_p, bb_active_o, bb_active_b, bb_active_c, bb_active_y, bb_active_blue;

reg [10:0] left_r, left_p, left_g, left_o, left_b, left_c, left_y, left_blue;
reg [10:0] right_r, right_p, right_g, right_o, right_b, right_c, right_y, right_blue;
reg [10:0] top_r, top_p, top_g, top_o, top_b, top_c, top_y, top_blue;
reg [10:0] bottom_r, bottom_p, bottom_g, bottom_o, bottom_b, bottom_c, bottom_y, bottom_blue;

assign bb_active_r = (x == left_r && left_r != IMAGE_W-11'h1) || (x == right_r && right_r != 0) || (y == top_r && top_r != IMAGE_H-11'h1) || (y == bottom_r && bottom_r != 0);
assign bb_active_p = (x == left_p && left_p != IMAGE_W-11'h1) || (x == right_p && right_p != 0) || (y == top_p && top_p != IMAGE_H-11'h1) || (y == bottom_p && bottom_p != 0);
assign bb_active_g = (x == left_g && left_g != IMAGE_W-11'h1) || (x == right_g && right_g != 0) || (y == top_g && top_g != IMAGE_H-11'h1) || (y == bottom_g && bottom_g != 0);
assign bb_active_o = (x == left_o && left_o != IMAGE_W-11'h1) || (x == right_o && right_o != 0) || (y == top_o && top_o != IMAGE_H-11'h1) || (y == bottom_o && bottom_o != 0);
assign bb_active_b = (x == left_b && left_b != IMAGE_W-11'h1) || (x == right_b && right_b != 0) || (y == top_b && top_b != IMAGE_H-11'h1) || (y == bottom_b && bottom_b != 0);
assign bb_active_c = (x == left_c && left_c != IMAGE_W-11'h1) || (x == right_c && right_c != 0) || (y == top_c && top_c != IMAGE_H-11'h1) || (y == bottom_c && bottom_c != 0);


// active r = x = left_r |  && red_detected 
assign new_image = 
//bb_active_edge ? {24'hf20b97} : 
//{h_edge_detected_final_shifted,h_edge_detected_final_shifted,h_edge_detected_final_shifted}; 
//{h_edge_detected_final_shifted,h_edge_detected_final_shifted,h_edge_detected_final_shifted}; 
 bb_active_r ? {24'hff0000} : 
 //bb_active_p ? {24'h00ff00} : 
 bb_active_g ? {24'h0000ff} : 
 bb_active_o ? {24'hf0f0f0} : 
 //bb_active_b ? {24'hff00ff} :
 bb_active_c ? {24'h2fbd9f}:
 bb_active_y ? {24'hede26f}:
 bb_active_blue ? {24'h51e5f4}:
 color_high; 

assign {red_out, green_out, blue_out} = (mode & ~sop & packet_video) ? new_image : 
{red,green,blue};




///////////////////////////////////////////////////////////////////////////////////////////////////
// Refined Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
//Count valid pixels to tget the image coordinates. Reset and detect packet type on Start of Packet.
reg [10:0] x, y;
reg packet_video;
//counting how many pixels detected are there in a row;
reg [10:0] count_r, count_p, count_o, count_g, count_b, count_c, count_y, count_blue;
//count how many pixels in this color between the edge gap
reg [10:0] max_start_edge_x_position_r,  max_start_edge_x_position_p, max_start_edge_x_position_o, max_start_edge_x_position_g, max_start_edge_x_position_b, max_start_edge_x_position_c, max_start_edge_x_position_y, max_start_edge_x_position_blue;
reg [10:0] max_end_edge_x_position_r, max_end_edge_x_position_p, max_end_edge_x_position_o, max_end_edge_x_position_g, max_end_edge_x_position_b, max_end_edge_x_position_c, max_end_edge_x_position_y, max_end_edge_x_position_blue;
// TODO: Refined with better math model
// The trusted region base on the previous X row
reg [10:0] estimatated_region_start_r, estimatated_region_end_r;
reg [10:0] estimatated_region_start_p, estimatated_region_end_p;
reg [10:0] estimatated_region_start_o, estimatated_region_end_o;
reg [10:0] estimatated_region_start_g, estimatated_region_end_g;
reg [10:0] estimatated_region_start_b, estimatated_region_end_b;
reg [10:0] estimatated_region_start_c, estimatated_region_end_c;
reg [10:0] estimatated_region_start_y, estimatated_region_end_y;
reg [10:0] estimatated_region_start_blue, estimatated_region_end_blue;

// The trusted meatrix
reg[7:0] estimated_val_r, estimated_val_p, estimated_val_o, estimated_val_g, estimated_val_b, estimated_val_c, estimated_val_y, estimated_val_blue;
wire [10:0] mid_deviation_r, mid_deviation_p, mid_deviation_o, mid_deviation_g, mid_deviation_b, mid_deviation_c, mid_deviation_y, mid_deviation_blue;
wire [10:0] difference_r, difference_p, difference_o, difference_g, difference_b, difference_c, difference_y, difference_blue;


// trusted y learned from the previous N rows
reg [10:0] estimated_y_min_r, estimated_y_min_p, estimated_y_min_g, estimated_y_min_o, estimated_y_min_b, estimated_y_min_c, estimated_y_min_y, estimated_y_min_blue;
reg [10:0] estimated_y_max_r, estimated_y_max_p, estimated_y_max_g, estimated_y_max_o, estimated_y_max_b, estimated_y_max_c, estimated_y_max_y, estimated_y_max_blue;


reg [7:0] estimated_val_y_max_r, estimated_val_y_max_p, estimated_val_y_max_g, estimated_val_y_max_o, estimated_val_y_max_b, estimated_val_y_max_c, estimated_val_y_max_y, estimated_val_y_max_blue;
reg [7:0] estimated_val_y_min_r, estimated_val_y_min_p, estimated_val_y_min_g, estimated_val_y_min_o, estimated_val_y_min_b, estimated_val_y_min_c, estimated_val_y_min_y, estimated_val_y_min_blue;
reg [10:0] immediate_y_min_r, immediate_y_min_p, immediate_y_min_g, immediate_y_min_o, immediate_y_min_b, immediate_y_min_c, immediate_y_min_y, immediate_y_min_blue;
reg [10:0] immediate_y_max_r, immediate_y_max_p, immediate_y_max_g, immediate_y_max_o, immediate_y_max_b, immediate_y_max_c, immediate_y_max_y, immediate_y_max_blue;



assign mid_deviation_r =  ((estimatated_region_end_r + estimatated_region_start_r) > (max_start_edge_x_position_r + max_end_edge_x_position_r)) ? 
							((estimatated_region_end_r + estimatated_region_start_r) - (max_start_edge_x_position_r + max_end_edge_x_position_r))
							: ((max_start_edge_x_position_r + max_end_edge_x_position_r) - ( estimatated_region_end_r + estimatated_region_start_r ));


assign mid_deviation_p =  ((estimatated_region_end_p + estimatated_region_start_p) > (max_start_edge_x_position_p + max_end_edge_x_position_p)) ? 
							((estimatated_region_end_p + estimatated_region_start_p) - (max_start_edge_x_position_p + max_end_edge_x_position_p))
							: ((max_start_edge_x_position_p + max_end_edge_x_position_p) - ( estimatated_region_end_p + estimatated_region_start_p ));


assign mid_deviation_o =  ((estimatated_region_end_o + estimatated_region_start_o) > (max_start_edge_x_position_o + max_end_edge_x_position_o)) ? 
							((estimatated_region_end_o + estimatated_region_start_o) - (max_start_edge_x_position_o + max_end_edge_x_position_o))
							: ((max_start_edge_x_position_o + max_end_edge_x_position_o) - ( estimatated_region_end_o + estimatated_region_start_o ));

assign mid_deviation_g =  ((estimatated_region_end_g + estimatated_region_start_g) > (max_start_edge_x_position_g + max_end_edge_x_position_g)) ? 
							((estimatated_region_end_g + estimatated_region_start_g) - (max_start_edge_x_position_g + max_end_edge_x_position_g))
							: ((max_start_edge_x_position_g + max_end_edge_x_position_g) - ( estimatated_region_end_g + estimatated_region_start_g ));

assign mid_deviation_b =  ((estimatated_region_end_b + estimatated_region_start_b) > (max_start_edge_x_position_b + max_end_edge_x_position_b)) ? 
							((estimatated_region_end_b + estimatated_region_start_b) - (max_start_edge_x_position_b + max_end_edge_x_position_b))
							: ((max_start_edge_x_position_b + max_end_edge_x_position_b) - ( estimatated_region_end_b + estimatated_region_start_b ));

assign mid_deviation_c =  ((estimatated_region_end_c + estimatated_region_start_c) > (max_start_edge_x_position_c + max_end_edge_x_position_c)) ? 
							((estimatated_region_end_c + estimatated_region_start_c) - (max_start_edge_x_position_c + max_end_edge_x_position_c))
							: ((max_start_edge_x_position_c + max_end_edge_x_position_c) - ( estimatated_region_end_c + estimatated_region_start_c ));

assign mid_deviation_y =  ((estimatated_region_end_y + estimatated_region_start_y) > (max_start_edge_x_position_y + max_end_edge_x_position_y)) ? 
							((estimatated_region_end_b + estimatated_region_start_y) - (max_start_edge_x_position_y + max_end_edge_x_position_y))
							: ((max_start_edge_x_position_b + max_end_edge_x_position_y) - ( estimatated_region_end_y + estimatated_region_start_y ));

assign mid_deviation_blue =  ((estimatated_region_end_blue + estimatated_region_start_blue) > (max_start_edge_x_position_blue + max_end_edge_x_position_blue)) ? 
							((estimatated_region_end_blue + estimatated_region_start_blue) - (max_start_edge_x_position_blue + max_end_edge_x_position_blue))
							: ((max_start_edge_x_position_blue + max_end_edge_x_position_blue) - ( estimatated_region_end_blue + estimatated_region_start_blue ));

assign difference_r = max_end_edge_x_position_r - max_start_edge_x_position_r;
assign difference_p = max_end_edge_x_position_p - max_start_edge_x_position_p;
assign difference_o = max_end_edge_x_position_o - max_start_edge_x_position_o;
assign difference_g = max_end_edge_x_position_g - max_start_edge_x_position_g;
assign difference_b = max_end_edge_x_position_b - max_start_edge_x_position_b;
assign difference_c = max_start_edge_x_position_c - max_end_edge_x_position_c;
assign difference_y = max_start_edge_x_position_y - max_end_edge_x_position_y;
assign difference_blue = max_start_edge_x_position_blue  - max_end_edge_x_position_blue;


always@(posedge clk) begin
    if (in_valid) begin
		//Cycle through message writer states once started
		if (msg_state != 2'b00) msg_state <= msg_state + 2'b01;
		
		if (eop & in_valid & packet_video) begin  //Ignore non-video packets
		//Latch edges for display overlay on next frame

			left_r <= x_min_r;
			right_r <= x_max_r;
			top_r <= y_max_r;
			bottom_r <= y_min_r;

			left_g <= x_min_g;
			right_g <= x_max_g;
			top_g <= y_max_g;
			bottom_g <= y_min_g;

			left_p <= x_min_p;
			right_p <= x_max_p;
			top_p <= y_max_p;
			bottom_p <= y_min_p;

			left_o <= x_min_o;
			right_o <= x_max_o;
			top_o <= y_max_o;
			bottom_o <= y_min_o;

            left_c <= x_min_c;
			right_c <= x_max_c;
			top_c <= y_max_c;
			bottom_c <= y_min_c;

            left_y <= x_min_y;
			right_y <= x_max_y;
			top_y <= y_max_y;
			bottom_y <= y_min_y;

            left_blue <= x_min_blue;
			right_blue <= x_max_blue;
			top_blue <= y_max_blue;
			bottom_blue <= y_min_blue;

            left_b <= x_min_b;
			right_b <= x_max_b;
			top_b <= y_max_b;
			bottom_b <= y_min_b;
			//end
			//keep last 4 values

			//red
			left_r_1 <= left_r;
			left_r_2 <= left_r_1;
			left_r_3 <= left_r_2;
			left_r_4 <= left_r_3;

			right_r_1 <= right_r;
			right_r_2 <= right_r_1;
			right_r_3 <= right_r_2;
			right_r_4 <= right_r_3;
			
			top_r_1 <= top_r;
			top_r_2 <= top_r_1;
			top_r_3 <= top_r_2;
			top_r_4 <= top_r_3;
			
			bottom_r_1 <= bottom_r;
			bottom_r_2 <= bottom_r_1;
			bottom_r_3 <= bottom_r_2;
			bottom_r_4 <= bottom_r_3;

			// pink
			left_p_1 <= left_p;
			left_p_2 <= left_p_1;
			left_p_3 <= left_p_2;
			left_p_4 <= left_p_3;

			right_p_1 <= right_p;
			right_p_2 <= right_p_1;
			right_p_3 <= right_p_2;
			right_p_4 <= right_p_3;
			
			top_p_1 <= top_p;
			top_p_2 <= top_p_1;
			top_p_3 <= top_p_2;
			top_p_4 <= top_p_3;
			
			bottom_p_1 <= bottom_p;
			bottom_p_2 <= bottom_p_1;
			bottom_p_3 <= bottom_p_2;
			bottom_p_4 <= bottom_p_3;
			

			//orange
			left_o_1 <= left_o;
			left_o_2 <= left_o_1;
			left_o_3 <= left_o_2;
			left_o_4 <= left_o_3;

			right_o_1 <= right_o;
			right_o_2 <= right_o_1;
			right_o_3 <= right_o_2;
			right_o_4 <= right_o_3;
			
			top_o_1 <= top_o;
			top_o_2 <= top_o_1;
			top_o_3 <= top_o_2;
			top_o_4 <= top_o_3;
			
			bottom_o_1 <= bottom_o;
			bottom_o_2 <= bottom_o_1;
			bottom_o_3 <= bottom_o_2;
			bottom_o_4 <= bottom_o_3;

			// green
			left_g_1 <= left_g;
			left_g_2 <= left_g_1;
			left_g_3 <= left_g_2;
			left_g_4 <= left_g_3;

			right_g_1 <= right_g;
			right_g_2 <= right_g_1;
			right_g_3 <= right_g_2;
			right_g_4 <= right_g_3;
			
			top_g_1 <= top_g;
			top_g_2 <= top_g_1;
			top_g_3 <= top_g_2;
			top_g_4 <= top_g_3;
			
			bottom_g_1 <= bottom_g;
			bottom_g_2 <= bottom_g_1;
			bottom_g_3 <= bottom_g_2;
			bottom_g_4 <= bottom_g_3;

			// black
			left_b_1 <= left_b;
			left_b_2 <= left_b_1;
			left_b_3 <= left_b_2;
			left_b_4 <= left_b_3;

			right_b_1 <= right_b;
			right_b_2 <= right_b_1;
			right_b_3 <= right_b_2;
			right_b_4 <= right_b_3;
			
			top_b_1 <= top_b;
			top_b_2 <= top_b_1;
			top_b_3 <= top_b_2;
			top_b_4 <= top_b_3;
			
			bottom_b_1 <= bottom_b;
			bottom_b_2 <= bottom_b_1;
			bottom_b_3 <= bottom_b_2;
			bottom_b_4 <= bottom_b_3;

            //cyan
            left_c_1 <= left_c;
			left_c_2 <= left_c_1;
			left_c_3 <= left_c_2;
			left_c_4 <= left_c_3;

			right_c_1 <= right_c;
			right_c_2 <= right_c_1;
			right_c_3 <= right_c_2;
			right_c_4 <= right_c_3;
			
			top_c_1 <= top_c;
			top_c_2 <= top_c_1;
			top_c_3 <= top_c_2;
			top_c_4 <= top_c_3;
			
			bottom_c_1 <= bottom_c;
			bottom_c_2 <= bottom_c_1;
			bottom_c_3 <= bottom_c_2;
			bottom_c_4 <= bottom_c_3;

            //yellow
            left_y_1 <= left_y;
			left_y_2 <= left_y_1;
			left_y_3 <= left_y_2;
			left_y_4 <= left_y_3;

			right_y_1 <= right_y;
			right_y_2 <= right_y_1;
			right_y_3 <= right_y_2;
			right_y_4 <= right_y_3;
			
			top_y_1 <= top_y;
			top_y_2 <= top_y_1;
			top_y_3 <= top_y_2;
			top_y_4 <= top_y_3;
			
			bottom_y_1 <= bottom_y;
			bottom_y_2 <= bottom_y_1;
			bottom_y_3 <= bottom_y_2;
			bottom_y_4 <= bottom_y_3;

            //blue
            left_blue_1 <= left_blue;
			left_blue_2 <= left_blue_1;
			left_blue_3 <= left_blue_2;
			left_blue_4 <= left_blue_3;

			right_blue_1 <= right_blue;
			right_blue_2 <= right_blue_1;
			right_blue_3 <= right_blue_2;
			right_blue_4 <= right_blue_3;
			
			top_blue_1 <= top_blue;
			top_blue_2 <= top_blue_1;
			top_blue_3 <= top_blue_2;
			top_blue_4 <= top_blue_3;
			
			bottom_blue_1 <= bottom_blue;
			bottom_blue_2 <= bottom_blue_1;
			bottom_blue_3 <= bottom_blue_2;
			bottom_blue_4 <= bottom_blue_3;
			
			//window for last frame, frame is refreshed every eop
			
			//Start message writer FSM once every MSG_INTERVAL frames, if there is room in the FIFO
			frame_count <= frame_count - 1;
			
			if (frame_count == 0 && msg_buf_size < MESSAGE_BUF_MAX - 3) begin
				msg_state <= 2'b01;
				frame_count <= MSG_INTERVAL-1;
			end
		end

		//parameter MESSAGE_BUF_MAX = 256 parameter MSG_INTERVAL = 6;
		else if (sop & in_valid) begin	//Reset bounds on start of packet
				//red
                x_min_r <= IMAGE_W-11'h1;
				x_max_r <= 0;
				y_min_r <= IMAGE_H-11'h1;
				y_max_r <= 0;
                //green
				x_min_g <= IMAGE_W-11'h1;
				x_max_g <= 0;
				y_min_g <= IMAGE_H-11'h1;
				y_max_g <= 0;
                //orange
				x_min_o <= IMAGE_W-11'h1;
				x_max_o <= 0;
				y_min_o <= IMAGE_H-11'h1;
				y_max_o <= 0;
                //pink
				x_min_p <= IMAGE_W-11'h1;
				x_max_p <= 0;
				y_min_p <= IMAGE_H-11'h1;
				y_max_p <= 0;
                //black
                x_min_b <= IMAGE_W-11'h1;
				x_max_b <= 0;
				y_min_b <= IMAGE_H-11'h1;
				y_max_b <= 0;
                //cyan
                x_min_c <= IMAGE_W-11'h1;
				x_max_c <= 0;
				y_min_c <= IMAGE_H-11'h1;
				y_max_c <= 0;
                //yellow
                x_min_y <= IMAGE_W-11'h1;
				x_max_y <= 0;
				y_min_y <= IMAGE_H-11'h1;
				y_max_y <= 0;
                //blue
                x_min_blue <= IMAGE_W-11'h1;
				x_max_blue <= 0;
				y_min_blue <= IMAGE_H-11'h1;
				y_max_blue <= 0;
                

				estimated_val_r <= 0;
				estimated_val_p <= 0;
				estimated_val_o <= 0;
				estimated_val_g <= 0;
				estimated_val_b <= 0;
                estimated_val_c <= 0;
				estimated_val_y <= 0;
				estimated_val_blue <= 0;
                
				count_r <= 0;
				count_p <= 0;
				count_g <= 0;
				count_o <= 0;
				count_b <= 0;
                count_c <= 0;
                count_y <= 0;
                count_blue <= 0;
                	
				estimated_val_y_min_r <= 0; estimated_val_y_min_p <= 0; estimated_val_y_min_g <= 0; estimated_val_y_min_o <= 0; estimated_val_y_min_b <= 0; estimated_val_y_min_c <= 0; estimated_val_y_min_y <= 0; estimated_val_y_min_blue <= 0;
				estimated_val_y_max_r <= 0; estimated_val_y_max_p <= 0; estimated_val_y_max_g <= 0; estimated_val_y_max_o <= 0; estimated_val_y_max_b <= 0; estimated_val_y_max_c <= 0; estimated_val_y_max_y <= 0; estimated_val_y_max_blue <= 0;
				immediate_y_min_r <= 0; immediate_y_min_p <= 0; immediate_y_min_g <= 0; immediate_y_min_o <= 0; immediate_y_min_b <= 0; immediate_y_min_c <= 0; immediate_y_min_y <= 0; immediate_y_min_blue <= 0;
				immediate_y_max_r <= 0; immediate_y_max_p <= 0; immediate_y_max_g <= 0; immediate_y_max_o <= 0; immediate_y_max_b <= 0; immediate_y_max_c <= 0; immediate_y_max_y <= 0; immediate_y_max_blue <= 0;
			end
	
	
		else begin					
				if(red_final_detected) 			count_r <= count_r + 1;
				else if (pink_final_detected) 	count_p <= count_p + 1;
				else if (green_final_detected) 	count_g <= count_g + 1;
				else if (orange_final_detected) count_o <= count_o + 1;
				else if (black_final_detected) 	count_b <= count_b + 1;
                else if (cyan_final_detected) 	count_c <= count_c + 1;
				else if (yellow_final_detected) count_y <= count_y + 1;
				else if (blue_final_detected) 	count_blue <= count_blue + 1;

				/////////////////////////////////////////////////
				// Row :: locating max_valid_region in a row or counting red..
				/////////////////////////////////////////////////

				//if (red_final_detected & in_valid & (h_edge_detected_final || h_edge_detected_final_1 || h_edge_detected_final_2)) begin	//Update bounds when the pixel is red
				if (red_final_detected & in_valid) begin	//Update bounds when the pixel is red
					if (x < max_start_edge_x_position_r) max_start_edge_x_position_r <= x;
					if (x > max_end_edge_x_position_r) max_end_edge_x_position_r <= x;
				end
				//else if (green_detected & green_final_detected & in_valid & (h_edge_detected_final || h_edge_detected_final_1 || h_edge_detected_final_2)) begin	//Update bounds when the pixel is red
				else if (green_final_detected & in_valid) begin
					if (x < max_start_edge_x_position_g) max_start_edge_x_position_g <= x;
					if (x > max_end_edge_x_position_g) max_end_edge_x_position_g <= x;
				end
				// else if (pink_detected & pink_final_detected & in_valid & (h_edge_detected_final || h_edge_detected_final_1 || h_edge_detected_final_2) & in_valid) begin	//Update bounds when the pixel is red		else if (pink_detected & pink_final_detected & in_valid & (h_edge_detected_final || h_edge_detected_final_1 || h_edge_detected_final_2) & in_valid) begin	//Update bounds when the pixel is red
				else if (pink_final_detected & in_valid) begin
					if (x < max_start_edge_x_position_p) max_start_edge_x_position_p <= x;
					if (x > max_end_edge_x_position_p) max_end_edge_x_position_p <= x;
				end	
				// else if (orange_detected & orange_final_detected & in_valid & (h_edge_detected_final || h_edge_detected_final_1 || h_edge_detected_final_2) & in_valid) begin	//Update bounds when the pixel is red
				else if (orange_final_detected & in_valid ) begin	//Update bounds when the pixel is red
					if (x < max_start_edge_x_position_o) max_start_edge_x_position_o <= x;
					if (x > max_end_edge_x_position_o) max_end_edge_x_position_o <= x;	
				end
				// else if (black_detected  & black_final_detected & in_valid & (h_edge_detected_final || h_edge_detected_final_1 || h_edge_detected_final_2) & in_valid) begin	//Update bounds when the pixel is red
				else if (black_final_detected & in_valid ) begin
					if (x < max_start_edge_x_position_b) max_start_edge_x_position_b <= x;
					if (x > max_end_edge_x_position_b) max_end_edge_x_position_b <= x;
				end
                else if (cyan_final_detected & in_valid ) begin
					if (x < max_start_edge_x_position_c) max_start_edge_x_position_c <= x;
					if (x > max_end_edge_x_position_c) max_end_edge_x_position_c <= x;
				end
                else if (yellow_final_detected & in_valid ) begin
					if (x < max_start_edge_x_position_y) max_start_edge_x_position_y <= x;
					if (x > max_end_edge_x_position_y) max_end_edge_x_position_y <= x;
				end
                else if (blue_final_detected & in_valid ) begin
					if (x < max_start_edge_x_position_blue) max_start_edge_x_position_blue <= x;
					if (x > max_end_edge_x_position_blue) max_end_edge_x_position_blue <= x;
				end
			end
		end


     		//////////////////////////////////////////////////////////////
			// Column :: 
     		//////////////////////////////////////////////////////////////
	if (x == IMAGE_W-1) begin
		count_r <= 0;
		count_p <= 0;
		count_g <= 0;
		count_o <= 0;
		count_b <= 0;	
        count_c <= 0;
        count_y <= 0;
        count_blue <= 0;
		max_start_edge_x_position_r <=  IMAGE_W-11'h1;
		max_end_edge_x_position_r <= 0;
		max_start_edge_x_position_p <=  IMAGE_W-11'h1;
		max_end_edge_x_position_p <= 0;
		max_start_edge_x_position_o <=  IMAGE_W-11'h1;
		max_end_edge_x_position_o <= 0;
		max_start_edge_x_position_g <=  IMAGE_W-11'h1;
		max_end_edge_x_position_g <= 0;
		max_start_edge_x_position_b <=  IMAGE_W-11'h1;
		max_end_edge_x_position_b <= 0;
        max_start_edge_x_position_c <=  IMAGE_W-11'h1;
		max_end_edge_x_position_c <= 0;
        max_start_edge_x_position_y <=  IMAGE_W-11'h1;
		max_end_edge_x_position_y <= 0;
        max_start_edge_x_position_blue <=  IMAGE_W-11'h1;
		max_end_edge_x_position_blue <= 0;

		// when the estimation is not valid
		//Red
		if(count_r > count_threshold )begin
			if(estimated_val_r == 0) begin
				estimatated_region_start_r <= max_start_edge_x_position_r;
				estimatated_region_end_r <= max_end_edge_x_position_r;
				// error choice, reset.
				x_min_r <= IMAGE_W-11'h1;
				x_max_r <= 0;
				y_min_r <= IMAGE_H-11'h1;
				y_max_r <= 0;
				estimated_val_r <= 1;
			end

			else begin
				if(mid_deviation_r > horizontal_edge_region_threshold)begin
					estimated_val_r <= estimated_val_r - 1;
				end
				else begin
					//discuss difference
					if(difference_r < difference_threshold )begin
						//valid row
						estimated_val_r <= estimated_val_r + 1;
						// choose the x region
						if(x_min_r > max_start_edge_x_position_r) begin
							x_min_r <= max_start_edge_x_position_r;
						end
						if(x_max_r < max_end_edge_x_position_r) begin
							x_max_r <= max_end_edge_x_position_r;
						end
						//////////////////////////////////////////////////////////////
						// Y
						//////////////////////////////////////////////////////////////
						if(estimated_val_y_min_r == 0)
						begin
								immediate_y_min_r <= y;
								y_min_r <= y;
								estimated_val_y_min_r <= 1;
						end
						else begin if(y - immediate_y_min_r > y_threshold) begin					
								estimated_val_y_min_r <= estimated_val_y_min_r - 1;
								//change 1
								//immediate_y_min_r <= y;
							end 
							else begin	
								estimated_val_y_min_r <= estimated_val_y_min_r + 1;	
								immediate_y_min_r <= y;							
							end
						end 

						// max					
						if(estimated_val_y_max_r  == 0) 
						begin
							immediate_y_max_r <= y;
							
							y_min_r <= y;
								
							estimated_val_y_max_r <= 1;
						end 
						else begin 
							if(y - immediate_y_max_r < y_threshold) begin
								estimated_val_y_max_r <= estimated_val_y_max_r + 1;
								immediate_y_max_r <= y;
								y_max_r <= y;
							end
							else begin
								//change 2
								//immediate_y_max_r <= y;
								estimated_val_y_max_r <= estimated_val_y_max_r - 1;
							end
						end

					end
				end
				//else do nothing
			end
		end

		//Pink
		if(count_p > count_threshold )begin
			if(estimated_val_p == 0)begin
				estimatated_region_start_p <= max_start_edge_x_position_p;
				estimatated_region_end_p <= max_end_edge_x_position_p;
				//reset
				left_p <= IMAGE_W-11'h1;
				right_p <= 0;
				top_p <= IMAGE_W-11'h1;
				bottom_p <= 0;
				estimated_val_p <= 1;
			end
			else begin
				if(mid_deviation_p > horizontal_edge_region_threshold)begin
					estimated_val_p <= estimated_val_p - 1;
				end
				else begin
					if(difference_p < difference_threshold )begin
						estimated_val_p <= estimated_val_p + 1;
						if(x_min_p > max_start_edge_x_position_p) begin
							x_min_p <= max_start_edge_x_position_p;
						end
						if(x_max_p < max_end_edge_x_position_p) begin
							x_max_p <= max_end_edge_x_position_p;
						end

						if(estimated_val_y_min_p == 0)
						begin
								immediate_y_min_p <= y;
								y_min_p <= y;
								estimated_val_y_min_p <= 1;
						end
						else begin if(y - immediate_y_min_p > y_threshold) begin					
								estimated_val_y_min_p <= estimated_val_y_min_p - 1;
							end 
							else begin	
								estimated_val_y_min_p <= estimated_val_y_min_p + 1;	
								immediate_y_min_p <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_p == 0) 
						begin
							immediate_y_max_p <= y;
							
							y_min_p <= y;
								
							estimated_val_y_max_p <= 1;
						end 
						else begin 
							if(y - immediate_y_max_p < y_threshold) begin
								estimated_val_y_max_p <= estimated_val_y_max_p + 1;
								immediate_y_max_p <= y;
								y_max_p <= y;
							end
							else begin
								estimated_val_y_max_p <= estimated_val_y_max_p - 1;
							end
						end
					end
				end
			end
		end

		//Orange
		if(count_o > count_threshold) begin
			if(estimated_val_o == 0)begin
				estimatated_region_start_o <= max_start_edge_x_position_o;
				estimatated_region_end_o <= max_end_edge_x_position_o;
				//reset
				x_min_o <= IMAGE_W-11'h1;
				x_max_o <= 0;
				y_min_o <= IMAGE_H-11'h1;
				y_max_o <= 0;
				estimated_val_o <= 1;
			end
			else begin
				if(mid_deviation_o > horizontal_edge_region_threshold)begin
					estimated_val_o <= estimated_val_o - 1;
				end
				else begin
					if(difference_o < difference_threshold)begin
						estimated_val_o <= estimated_val_o + 1;
						if(x_min_o > max_start_edge_x_position_o) begin
							x_min_o <= max_start_edge_x_position_o;
						end
						if(x_max_o < max_end_edge_x_position_o) begin
							x_max_o <= max_end_edge_x_position_o;
						end


																					
						if(estimated_val_y_min_o == 0)
						begin
								immediate_y_min_o <= y;
								y_min_o <= y;
								estimated_val_y_min_o <= 1;
						end
						else begin if(y - immediate_y_min_o > y_threshold) begin					
								estimated_val_y_min_o <= estimated_val_y_min_o - 1;
							end 
							else begin	
								estimated_val_y_min_o <= estimated_val_y_min_o + 1;	
								immediate_y_min_o <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_o  == 0) 
						begin
							immediate_y_max_o <= y;
							
							y_min_o <= y;
								
							estimated_val_y_max_o <= 1;
						end 
						else begin 
							if(y - immediate_y_max_o < y_threshold) begin
								estimated_val_y_max_o <= estimated_val_y_max_o + 1;
								immediate_y_max_o <= y;
								y_max_o <= y;
							end
							else begin
								estimated_val_y_max_o <= estimated_val_y_max_o - 1;
							end
						end

					end
				end
			end
		end
	
		//Green
		if(count_g > count_threshold) begin
			if(estimated_val_g == 0)begin
				estimatated_region_start_g <= max_start_edge_x_position_g;
				estimatated_region_end_g <= max_end_edge_x_position_g;
				//reset
				x_min_g <= IMAGE_W-11'h1;
				x_max_g <= 0;
				y_min_g <= IMAGE_H-11'h1;
				y_max_g <= 0;
				estimated_val_g <= 1;

			end
			else begin
				if(mid_deviation_g > horizontal_edge_region_threshold)begin
					estimated_val_g <= estimated_val_g - 1;
				end
				else begin
					if(difference_g < difference_threshold )begin
						estimated_val_g <= estimated_val_g + 1;
						if(x_min_g > max_start_edge_x_position_g) begin
							x_min_g <= max_start_edge_x_position_g;
						end
						if(x_max_g < max_end_edge_x_position_g) begin
							x_max_g <= max_end_edge_x_position_g;
						end
						if(estimated_val_y_min_g == 0)
						begin
								immediate_y_min_g <= y;
								y_min_g <= y;
								estimated_val_y_min_g <= 1;
						end
						else begin if(y - immediate_y_min_g > y_threshold) begin					
								estimated_val_y_min_g <= estimated_val_y_min_g - 1;
							end 
							else begin	
								estimated_val_y_min_g <= estimated_val_y_min_g + 1;	
								immediate_y_min_g <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_g  == 0) 
						begin
							immediate_y_max_g <= y;
							
							y_min_g <= y;
								
							estimated_val_y_max_g <= 1;
						end 
						else begin 
							if(y - immediate_y_max_g < y_threshold) begin
								estimated_val_y_max_g <= estimated_val_y_max_g + 1;
								immediate_y_max_g <= y;
								y_max_g <= y;
							end
							else begin
								estimated_val_y_max_g <= estimated_val_y_max_g - 1;
							end
						end

					end
				end
			end
		end

		//Black
		if(count_b > count_threshold) begin
			if(estimated_val_b == 0)begin
				estimatated_region_start_b <= max_start_edge_x_position_b;
				estimatated_region_end_b <= max_end_edge_x_position_b;
				//reset
				x_min_b <= IMAGE_W-11'h1;
				x_max_b <= 0;
				y_min_b <= IMAGE_H-11'h1;
				y_max_b <= 0;
				estimated_val_b <= 1;
			end
			else begin
				if(mid_deviation_b > horizontal_edge_region_threshold)begin
					estimated_val_b <= estimated_val_b - 1;
				end
				else begin
					if(difference_b < difference_threshold )begin
						estimated_val_b <= estimated_val_b + 1;
						if(x_min_b > max_start_edge_x_position_b) begin
							x_min_b <= max_start_edge_x_position_b;
						end
						if(x_max_b < max_end_edge_x_position_b) begin
							x_max_b <= max_end_edge_x_position_b;
						end
						// choose y region
						if(estimated_val_y_min_b == 0)
						begin
								immediate_y_min_b <= y;
								y_min_b <= y;
								estimated_val_y_min_b <= 1;
						end
						else begin if(y - immediate_y_min_b > y_threshold) begin					
								estimated_val_y_min_b <= estimated_val_y_min_b - 1;
							end 
							else begin	
								estimated_val_y_min_b <= estimated_val_y_min_b + 1;	
								immediate_y_min_b <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_b == 0) 
						begin
							immediate_y_max_b <= y;
							
							y_min_b <= y;
								
							estimated_val_y_max_b <= 1;
						end 
						else begin 
							if(y - immediate_y_max_b < y_threshold) begin
								estimated_val_y_max_b <= estimated_val_y_max_b + 1;
								immediate_y_max_b <= y;
								y_max_b <= y;
							end
							else begin
								estimated_val_y_max_b <= estimated_val_y_max_b - 1;
							end
						end

					end
				end
			end
		end

        //cyan
        if(count_c > count_threshold) begin
			if(estimated_val_c == 0)begin
				estimatated_region_start_c <= max_start_edge_x_position_c;
				estimatated_region_end_c <= max_end_edge_x_position_c;
				//reset
				x_min_c <= IMAGE_W-11'h1;
				x_max_c <= 0;
				y_min_c <= IMAGE_H-11'h1;
				y_max_c <= 0;
				estimated_val_c <= 1;
			end
			else begin
				if(mid_deviation_c > horizontal_edge_region_threshold)begin
					estimated_val_c <= estimated_val_c - 1;
				end
				else begin
					if(difference_c < difference_threshold )begin
						estimated_val_c <= estimated_val_c + 1;
						if(x_min_c > max_start_edge_x_position_c) begin
							x_min_c <= max_start_edge_x_position_c;
						end
						if(x_max_c < max_end_edge_x_position_c) begin
							x_max_c <= max_end_edge_x_position_c;
						end
						// choose y region
						if(estimated_val_y_min_c == 0)
						begin
								immediate_y_min_c <= y;
								y_min_c <= y;
								estimated_val_y_min_c <= 1;
						end
						else begin if(y - immediate_y_min_c > y_threshold) begin					
								estimated_val_y_min_c <= estimated_val_y_min_c - 1;
							end 
							else begin	
								estimated_val_y_min_c <= estimated_val_y_min_c + 1;	
								immediate_y_min_c <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_c == 0) 
						begin
							immediate_y_max_c <= y;
							
							y_min_c <= y;
								
							estimated_val_y_max_c <= 1;
						end 
						else begin 
							if(y - immediate_y_max_c < y_threshold) begin
								estimated_val_y_max_c <= estimated_val_y_max_c + 1;
								immediate_y_max_c <= y;
								y_max_c <= y;
							end
							else begin
								estimated_val_y_max_c <= estimated_val_y_max_c - 1;
							end
						end
					end
				end
			end
		end

        // yellow
        if(count_y > count_threshold) begin
			if(estimated_val_y == 0)begin
				estimatated_region_start_y <= max_start_edge_x_position_b;
				estimatated_region_end_y <= max_end_edge_x_position_b;
				//reset
				x_min_y <= IMAGE_W-11'h1;
				x_max_y <= 0;
				y_min_y <= IMAGE_H-11'h1;
				y_max_y <= 0;
				estimated_val_y <= 1;
			end
			else begin
				if(mid_deviation_y > horizontal_edge_region_threshold)begin
					estimated_val_y <= estimated_val_y - 1;
				end
				else begin
					if(difference_y < difference_threshold )begin
						estimated_val_y <= estimated_val_y + 1;
						if(x_min_y > max_start_edge_x_position_y) begin
							x_min_y <= max_start_edge_x_position_y;
						end
						if(x_max_y < max_end_edge_x_position_y) begin
							x_max_y <= max_end_edge_x_position_y;
						end
						// choose y region
						if(estimated_val_y_min_y == 0)
						begin
								immediate_y_min_y <= y;
								y_min_b <= y;
								estimated_val_y_min_y <= 1;
						end
						else begin if(y - immediate_y_min_y > y_threshold) begin					
								estimated_val_y_min_y <= estimated_val_y_min_y - 1;
							end 
							else begin	
								estimated_val_y_min_y <= estimated_val_y_min_y + 1;	
								immediate_y_min_y <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_y == 0) 
						begin
							immediate_y_max_y <= y;
							
							y_min_y <= y;
								
							estimated_val_y_max_y <= 1;
						end 
						else begin 
							if(y - immediate_y_max_y < y_threshold) begin
								estimated_val_y_max_y <= estimated_val_y_max_y + 1;
								immediate_y_max_y <= y;
								y_max_b <= y;
							end
							else begin
								estimated_val_y_max_y <= estimated_val_y_max_y - 1;
							end
						end

					end
				end
			end
		end

        //blue
        if(count_blue > count_threshold) begin
			if(estimated_val_blue == 0)begin
				estimatated_region_start_blue <= max_start_edge_x_position_blue;
				estimatated_region_end_blue <= max_end_edge_x_position_blue;
				//reset
				x_min_blue <= IMAGE_W-11'h1;
				x_max_blue <= 0;
				y_min_blue <= IMAGE_H-11'h1;
				y_max_blue <= 0;
				estimated_val_blue <= 1;
			end
			else begin
				if(mid_deviation_blue > horizontal_edge_region_threshold)begin
					estimated_val_blue <= estimated_val_blue - 1;
				end
				else begin
					if(difference_blue < difference_threshold )begin
						estimated_val_blue <= estimated_val_blue + 1;
						if(x_min_blue > max_start_edge_x_position_blue) begin
							x_min_blue <= max_start_edge_x_position_blue;
						end
						if(x_max_blue < max_end_edge_x_position_blue) begin
							x_max_blue <= max_end_edge_x_position_blue;
						end
						// choose y region
						if(estimated_val_y_min_blue == 0)
						begin
								immediate_y_min_blue <= y;
								y_min_blue <= y;
								estimated_val_y_min_blue <= 1;
						end
						else begin if(y - immediate_y_min_blue > y_threshold) begin					
								estimated_val_y_min_blue <= estimated_val_y_min_blue - 1;
							end 
							else begin	
								estimated_val_y_min_blue <= estimated_val_y_min_blue + 1;	
								immediate_y_min_blue <= y;							
							end
						end 

						// max	
							
						if(estimated_val_y_max_blue == 0) 
						begin
							immediate_y_max_blue <= y;
							
							y_min_blue <= y;
								
							estimated_val_y_max_blue <= 1;
						end 
						else begin 
							if(y - immediate_y_max_blue < y_threshold) begin
								estimated_val_y_max_blue <= estimated_val_y_max_blue + 1;
								immediate_y_max_blue <= y;
								y_max_blue <= y;
							end
							else begin
								estimated_val_y_max_blue <= estimated_val_y_max_blue - 1;
							end
						end

					end
				end
			end
		end
	end
end
		




always@(posedge clk) begin
	if (sop) begin
		x <= 11'h0;
		y <= 11'h0;
		packet_video <= (blue[3:0] == 3'h0);
	end
	else if (in_valid) begin
		if (x == IMAGE_W-1) begin
			x <= 11'h0;
			y <= y + 11'h1;
		end
		else begin
			x <= x + 11'h1;
		end
	end
end

// x,y represent position of a single pixel. Every clk 1 new pixel coming in.


//Find first and last red pixels
reg [10:0] x_min, y_min, x_max, y_max;
reg [10:0] x_min_r, x_min_p, x_min_g, x_min_o, x_min_b, x_min_c, x_min_y, x_min_blue;
reg [10:0] y_min_r, y_min_p, y_min_g, y_min_o, y_min_b, y_min_c, y_min_y, y_min_blue;
reg [10:0] x_max_r, x_max_p, x_max_g, x_max_o, x_max_b, x_max_c, x_max_y, x_max_blue;
reg [10:0] y_max_r, y_max_p, y_max_g, y_max_o, y_max_b, y_max_c, y_max_y, y_max_blue;

//Process bounding box at the end of the frame.
reg [1:0] msg_state;
reg [7:0] frame_count;

reg [10:0] left_r_1, left_r_2, left_r_3, left_r_4;
reg [10:0] right_r_1, right_r_2, right_r_3, right_r_4;
reg [10:0] top_r_1, top_r_2, top_r_3, top_r_4;
reg [10:0] bottom_r_1, bottom_r_2, bottom_r_3, bottom_r_4;

reg [10:0] left_p_1, left_p_2, left_p_3, left_p_4;
reg [10:0] right_p_1, right_p_2, right_p_3, right_p_4;
reg [10:0] top_p_1, top_p_2, top_p_3, top_p_4;
reg [10:0] bottom_p_1, bottom_p_2, bottom_p_3, bottom_p_4;

reg [10:0] left_o_1, left_o_2, left_o_3, left_o_4;
reg [10:0] right_o_1, right_o_2, right_o_3, right_o_4;
reg [10:0] top_o_1, top_o_2, top_o_3, top_o_4;
reg [10:0] bottom_o_1, bottom_o_2, bottom_o_3, bottom_o_4;

reg [10:0] left_g_1, left_g_2, left_g_3, left_g_4;
reg [10:0] right_g_1, right_g_2, right_g_3, right_g_4;
reg [10:0] top_g_1, top_g_2, top_g_3, top_g_4;
reg [10:0] bottom_g_1, bottom_g_2, bottom_g_3, bottom_g_4;

reg [10:0] left_b_1, left_b_2, left_b_3, left_b_4;
reg [10:0] right_b_1, right_b_2, right_b_3, right_b_4;
reg [10:0] top_b_1, top_b_2, top_b_3, top_b_4;
reg [10:0] bottom_b_1, bottom_b_2, bottom_b_3, bottom_b_4;
// Yellow
reg [10:0] left_y_1, left_y_2, left_y_3, left_y_4;
reg [10:0] right_y_1, right_y_2, right_y_3, right_y_4;
reg [10:0] top_y_1, top_y_2, top_y_3, top_y_4;
reg [10:0] bottom_y_1, bottom_y_2, bottom_y_3, bottom_y_4;
// cy
reg [10:0] left_c_1, left_c_2, left_c_3, left_c_4;
reg [10:0] right_c_1, right_c_2, right_c_3, right_c_4;
reg [10:0] top_c_1, top_c_2, top_c_3, top_c_4;
reg [10:0] bottom_c_1, bottom_c_2, bottom_c_3, bottom_c_4;
// blue
reg [10:0] left_blue_1, left_blue_2, left_blue_3, left_blue_4;
reg [10:0] right_blue_1, right_blue_2, right_blue_3, right_blue_4;
reg [10:0] top_blue_1, top_blue_2, top_blue_3, top_blue_4;
reg [10:0] bottom_blue_1, bottom_blue_2, bottom_blue_3, bottom_blue_4;

always@(posedge clk) begin
	
end



wire [10:0] avg_left_r, avg_right_r, avg_top_r, avg_bottom_r;
wire [10:0] avg_left_p, avg_right_p, avg_top_p, avg_bottom_p;
wire [10:0] avg_left_o, avg_right_o, avg_top_o, avg_bottom_o;
wire [10:0] avg_left_g, avg_right_g, avg_top_g, avg_bottom_g;
wire [10:0] avg_left_b, avg_right_b, avg_top_b, avg_bottom_b;
wire [10:0] avg_left_c, avg_right_c, avg_top_c, avg_bottom_c;
wire [10:0] avg_left_y, avg_right_y, avg_top_y, avg_bottom_y;
wire [10:0] avg_left_blue, avg_right_blue, avg_top_blue, avg_bottom_blue;


assign avg_left_r = (left_r + left_r_1 + left_r_2 + left_r_3 + left_r_4) / 5;
assign avg_right_r = (right_r + right_r_1 + right_r_2 + right_r_3 + left_r_4) / 5;
assign avg_top_r = (top_r + top_r_1 + top_r_2 + top_r_3 + top_r_4) / 5;
assign avg_bottom_r = (bottom_r + bottom_r_1 + bottom_r_2 + bottom_r_3 + bottom_r_4) / 5;

assign avg_left_p = (left_p + left_p_1 + left_p_2 + left_p_3 + left_p_4) / 5;
assign avg_right_p = (right_p + right_p_1 + right_p_2 + right_p_3 + left_p_4) / 5;
assign avg_top_p = (top_p + top_p_1 + top_p_2 + top_p_3 + top_p_4) / 5;
assign avg_bottom_p = (bottom_p + bottom_p_1 + bottom_p_2 + bottom_p_3 + bottom_p_4) / 5;

assign avg_left_o = (left_o + left_o_1 + left_o_2 + left_o_3 + left_o_4) / 5;
assign avg_right_o = (right_o + right_o_1 + right_o_2 + right_o_3 + left_o_4) / 5;
assign avg_top_o = (top_o + top_o_1 + top_o_2 + top_o_3 + top_o_4) / 5;
assign avg_bottom_o = (bottom_o + bottom_o_1 + bottom_o_2 + bottom_o_3 + bottom_o_4) / 5;

assign avg_left_g = (left_g + left_g_1 + left_g_2 + left_g_3 + left_g_4) / 5;
assign avg_right_g = (right_g + right_g_1 + right_g_2 + right_g_3 + left_g_4) / 5;
assign avg_top_g = (top_g + top_g_1 + top_g_2 + top_g_3 + top_g_4) / 5;
assign avg_bottom_g = (bottom_g + bottom_g_1 + bottom_g_2 + bottom_g_3 + bottom_g_4) / 5;

assign avg_left_b = (left_b + left_b_1 + left_b_2 + left_b_3 + left_b_4) / 5;
assign avg_right_b = (right_b + right_b_1 + right_b_2 + right_b_3 + left_b_4) / 5;
assign avg_top_b = (top_b + top_b_1 + top_b_2 + top_b_3 + top_b_4) / 5;
assign avg_bottom_b = (bottom_b + bottom_b_1 + bottom_b_2 + bottom_b_3 + bottom_b_4) / 5;
//yellow
assign avg_left_y = (left_y + left_y_1 + left_y_2 + left_y_3 + left_y_4) / 5;
assign avg_right_y = (right_y + right_y_1 + right_y_2 + right_y_3 + left_y_4) / 5;
assign avg_top_y = (top_y + top_y_1 + top_y_2 + top_y_3 + top_y_4) / 5;
assign avg_bottom_y = (bottom_y + bottom_y_1 + bottom_y_2 + bottom_y_3 + bottom_y_4) / 5;
//cy
assign avg_left_c = (left_c + left_c_1 + left_c_2 + left_c_3 + left_c_4) / 5;
assign avg_right_c = (right_c + right_c_1 + right_c_2 + right_c_3 + left_c_4) / 5;
assign avg_top_c = (top_c + top_c_1 + top_c_2 + top_c_3 + top_c_4) / 5;
assign avg_bottom_c = (bottom_c + bottom_c_1 + bottom_c_2 + bottom_c_3 + bottom_c_4) / 5;
//blue
assign avg_left_blue = (left_blue + left_blue_1 + left_blue_2 + left_blue_3 + left_blue_4) / 5;
assign avg_right_blue = (right_blue + right_blue_1 + right_blue_2 + right_blue_3 + left_blue_4) / 5;
assign avg_top_blue = (top_blue + top_blue_1 + top_blue_2 + top_blue_3 + top_blue_4) / 5;
assign avg_bottom_blue = (bottom_blue + bottom_blue_1 + bottom_blue_2 + bottom_blue_3 + bottom_blue_4) / 5;
	
	
//Generate output messages for CPU
reg [31:0] msg_buf_in;  
wire [31:0] msg_buf_out;
reg msg_buf_wr;
wire msg_buf_rd, msg_buf_flush;
wire [7:0] msg_buf_size;
wire msg_buf_empty;

`define RED_BOX_MSG_ID "RBB"

always@(*) begin	//Write words to FIFO as state machine advances
	case(msg_state)
		2'b00: begin
			msg_buf_in = 32'b0;
			msg_buf_wr = 1'b0;
		end
		2'b01: begin
			//msg_buf_in = 
			msg_buf_in = {24'b0, hue};
			//`RED_BOX_MSG_ID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		2'b10: begin
			//msg_buf_in = {5'b0, x_min, 5'b0, y_min};	//Top left coordinate
			msg_buf_in = {22'h0,hue}; 
			msg_buf_wr = 1'b1;
		end
		2'b11: begin
			msg_buf_in = {32'h0}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
	endcase
end

//Output message FIFO
MSG_FIFO	MSG_FIFO_inst (
	.clock (clk),
	.data (msg_buf_in),
	.rdreq (msg_buf_rd),
	.sclr (~reset_n | msg_buf_flush),
	.wrreq (msg_buf_wr),
	.q (msg_buf_out),
	.usedw (msg_buf_size),
	.empty (msg_buf_empty)
	);


//Streaming registers to buffer video signal
STREAM_REG #(.DATA_WIDTH(26)) in_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(sink_ready),
	.valid_out(in_valid),
	.data_out({red,green,blue,sop,eop}),
	.ready_in(out_ready),
	.valid_in(sink_valid),
	.data_in({sink_data,sink_sop,sink_eop})
);

STREAM_REG #(.DATA_WIDTH(26)) out_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(out_ready),
	.valid_out(source_valid),
	.data_out({source_data,source_sop,source_eop}),
	.ready_in(source_ready),
	.valid_in(in_valid),
	.data_in({red_out, green_out, blue_out, sop, eop})
);


/////////////////////////////////
/// Memory-mapped port		 /////
/////////////////////////////////

// Addresses
`define REG_STATUS    			0
`define READ_MSG    				1
`define READ_ID    				2
`define REG_BBCOL					3


reg  [7:0]   reg_status;
reg	[23:0]	bb_col;

always @ (posedge clk)
begin
	if (~reset_n)
	begin
		reg_status <= 8'b0;
		bb_col <= BB_COL_DEFAULT;
	end
	else begin
		if(s_chipselect & s_write) begin
		   if      (s_address == `REG_STATUS)	reg_status <= s_writedata[7:0];
		   if      (s_address == `REG_BBCOL)	bb_col <= s_writedata[23:0];
		end
	end
end



//Flush the message buffer if 1 is written to status register bit 4
assign msg_buf_flush = (s_chipselect & s_write & (s_address == `REG_STATUS) & s_writedata[4]);


// Process reads
reg read_d; //Store the read signal for correct updating of the message buffer

// Copy the requested word to the output port when there is a read.
always @ (posedge clk) begin
   if (~reset_n) begin
	   s_readdata <= {32'b0};
		read_d <= 1'b0;
	end
	
	else if (s_chipselect & s_read) begin
		if   (s_address == `REG_STATUS) s_readdata <= {16'b0,msg_buf_size,reg_status};
		if   (s_address == `READ_MSG) s_readdata <= {msg_buf_out};
		if   (s_address == `READ_ID) s_readdata <= 32'h1234EEE2;
		if   (s_address == `REG_BBCOL) s_readdata <= {8'h0, bb_col};
	end
	
	read_d <= s_read;
end

//Fetch next word from message buffer after read from READ_MSG
assign msg_buf_rd = s_chipselect & s_read & ~read_d & ~msg_buf_empty & (s_address == `READ_MSG);
////////////////////////////////////////////////////////////////////
// SPI Transimitioin
////////////////////////////////////////////////////////////////////
// data formate 
// colour + coordinate  = {0'b0, colour(3 bits), 12 bits for x_coordinate}
// colour + distance    = {0'b1, colour(3 bits), 12 bits for distance    }

wire formate_r, formate_p, formate_g, formate_o, formate_b, formate_c, formate_y, formate_blue ;
reg [10:0] distance_r, distance_p, distance_g, distance_o, distance_b, distance_c, distance_y, distance_blue;
reg valid_r, valid_p, valid_g, valid_o, valid_b, valid_c, valid_y, valid_blue;
reg valid_r_1, valid_p_1, valid_g_1, valid_o_1, valid_b_1, valid_y_1, valid_c_1, valid_blue_1;
reg valid_r_2, valid_p_2, valid_g_2, valid_o_2, valid_b_2, valid_y_2, valid_c_2, valid_blue_2;
reg valid_r_3, valid_p_3, valid_g_3, valid_o_3, valid_b_3, valid_y_3, valid_c_3, valid_blue_3;
wire [10:0] red_center_x_pixel, pink_center_x_pixel, green_center_x_pixel, orange_center_x_pixel, black_center_x_pixel, blue_center_x_pixel, yellow_center_x_pixel, cyan_center_x_pixel;

distance_cal red_ball( 
	.left_bound(avg_left_r), 
	.right_bound(avg_right_r),
	.upper_bound(avg_top_r),
	.eop(eop),
	.valid(valid_r),
    .formate(formate_r), 
	.target_center_x_pixel(red_center_x_pixel),
	.distance(distance_r) 
);
distance_cal pink_ball( 
	.left_bound(avg_left_p),
	.right_bound(avg_right_p),
	.upper_bound(avg_top_p),
	.eop(eop),
	.valid(valid_p),
	.formate(formate_p), 
	.target_center_x_pixel(pink_center_x_pixel),
	.distance(distance_p) 
);
distance_cal green_ball(
    .left_bound(avg_left_g),
    .right_bound(avg_right_g),
	.upper_bound(avg_top_g),
	.eop(eop),
	.valid(valid_g),
    .formate(formate_g),
    .target_center_x_pixel(green_center_x_pixel),
	.distance(distance_g) 
);

distance_cal orange_ball(
    .left_bound(avg_left_o),
    .right_bound(avg_right_o),
	.upper_bound(avg_top_o),
	.eop(eop),
	.valid(valid_o),
    .formate(formate_o),
    .target_center_x_pixel(orange_center_x_pixel),
	.distance(distance_o) 
);

// TODO: building distance
distance_cal cyan_ball(
    .left_bound(avg_left_c),
    .right_bound(avg_right_c),
	.upper_bound(avg_top_c),
	.eop(eop),
	.valid(valid_c),
    .formate(formate_c),
    .target_center_x_pixel(cyan_center_x_pixel),
	.distance(distance_c) 
);

distance_cal yellow_ball(
    .left_bound(avg_left_y),
    .right_bound(avg_right_y),
	.upper_bound(avg_top_y),
	.eop(eop),
	.valid(valid_y),
    .formate(formate_y),
    .target_center_x_pixel(yellow_center_x_pixel),
	.distance(distance_y) 
);
distance_cal blue_ball(
    .left_bound(avg_left_blue),
    .right_bound(avg_right_blue),
	.upper_bound(avg_top_blue),
	.eop(eop),
	.valid(valid_blue),
    .formate(formate_blue),
    .target_center_x_pixel(blue_center_x_pixel),
	.distance(distance_blue) 
);

wire [11:0] c_1,c_2,c_3,c_4,c_5;


reg selected_r, selected_p, selected_o, selected_b, selected_g, selected_c, selected_y, selected_blue;
// reg [15:0] message_to_ESP32;ss

// minmum distance;
assign c_1 = (valid_r && ~selected_r)? distance_r : 12'b111111111111;
assign c_2 = (0 && distance_p < c_1 && ~selected_p) ? distance_p : c_1;
assign c_3 = (0 && distance_g < c_2 && ~selected_g) ? distance_g : c_2;
assign c_4 = (valid_o && distance_o < c_3 && ~selected_o) ? distance_o : c_3;
assign c_5 = (0 && distance_b < c_4 && ~selected_b) ? distance_b : c_4;
assign c_6 = (0 && distance_y < c_5 && ~selected_y) ? distance_y : c_5;
assign c_7 = (0 && distance_c < c_6 && ~selected_c) ? distance_c : c_6;
assign c_8 = (0 && distance_blue < c_7 && ~selected_blue) ? distance_blue : c_7;

//wire [2:0] data_colour;
// assign data_colour = (lock_r) ? 3'b000: 
// 					 (lock_p) ? 3'b001: 
// 					 (lock_g) ? 3'b010: 
// 					 (lock_o) ? 3'b011: 
// 					 (lock_b) ? 3'b100: 
// 					 (c_5 == distance_r) ? 3'b000 :
// 					 (c_5 == distance_p) ? 3'b001 :
// 					 (c_5 == distance_g) ? 3'b010 :
// 					 (c_5 == distance_o) ? 3'b011 :
// 					 (c_5 == distance_b) ? 3'b100 : 3'b111;		

reg [3:0] data_colour; 
reg moving_forward_r, moving_forward_p, moving_forward_g, moving_forward_o, moving_forward_b, moving_forward_c, moving_forward_y, moving_forward_blue;
reg detection_request;

always @(posedge clk) begin
	if(eop) begin
		valid_r_1 <= valid_r;
		valid_r_2 <= valid_r_1;
		valid_r_3 <= valid_r_2;

		valid_p_1 <= valid_p;
		valid_p_2 <= valid_p_1;
		valid_p_3 <= valid_p_2;

		valid_o_1 <= valid_o;
		valid_o_2 <= valid_o_1;
		valid_o_3 <= valid_o_2;
		
		valid_g_1 <= valid_g;
		valid_g_2 <= valid_g_1;
		valid_g_3 <= valid_g_2;
		
		valid_b_1 <= valid_b;
		valid_b_2 <= valid_b_1;
		valid_b_3 <= valid_b_2;
        
        valid_c_1 <= valid_c;
		valid_c_2 <= valid_c_1;
		valid_c_3 <= valid_c_2;
        
        valid_y_1 <= valid_y;
		valid_y_2 <= valid_y_1;
		valid_y_3 <= valid_y_2;

        valid_blue_1 <= valid_blue;
		valid_blue_2 <= valid_blue_1;
		valid_blue_3 <= valid_blue_2;
	end
end

always @(posedge clk) begin
	//esp32 has successfully received red distance. red is now in the selected set

	// unlock the target and block the target.
	if(message_from_ESP32 == 50) begin
		moving_forward_r <= 1;
		moving_forward_p <= 1;
		moving_forward_g <= 1;
		moving_forward_o <= 1;
		moving_forward_b <= 1;
        moving_forward_y <= 1;
        moving_forward_c <= 1;
        moving_forward_blue <= 1;
		detection_request <= 0;
	end
	// 1 moving forward

	//if(message_from_ESP32 == 21) moving_forward_or_rotate <= 0;
	// 0 rotate 
	
	else if(message_from_ESP32 == 30) begin
		selected_r <= 1;
	end 
	else if(message_from_ESP32 == 31) begin
		selected_p <= 1;
	end 
	else if(message_from_ESP32 == 32) begin
		selected_g <= 1;
	end 
	else if(message_from_ESP32 == 33) begin
		selected_o <= 1;
	end 
	else if(message_from_ESP32 == 34) begin
		selected_b <= 1;
	end 
    else if(message_from_ESP32 == 35) begin
		selected_c <= 1;
	end 
    else if(message_from_ESP32 == 36) begin
		selected_y <= 1;
	end 
    else if(message_from_ESP32 == 37) begin
		selected_blue <= 1;
	end 
	else if(message_from_ESP32 == 70)begin
			selected_r <= 0;
			selected_p <= 0;
			selected_o <= 0;
			selected_g <= 0;
			selected_b <= 0;
            selected_y <= 0;
            selected_c <= 0;
            selected_blue <= 0;
			moving_forward_r <= 0;
			moving_forward_p <= 0;
			moving_forward_o <= 0;
			moving_forward_g <= 0;
			moving_forward_b <= 0;
            moving_forward_c <= 0;
            moving_forward_y <= 0;
            moving_forward_blue <= 0;

			//1507 data colour
			//1535 for lock
	end 
	else if(message_from_ESP32 == 100) begin
		detection_request <= 1;	
	end
	else begin
		if(~valid_r && ~valid_r_1 && ~valid_r_2 && ~valid_r_3 && moving_forward_r)begin 
			selected_r <= 0; 
			moving_forward_r <= 0;
		end
		if(~valid_p && ~valid_p_1 && ~valid_p_2 && ~valid_p_3 && moving_forward_p) begin 
			selected_p <= 0; 
			moving_forward_p <= 0;
		end  
		if(~valid_g && ~valid_g_1 && ~valid_g_2 && ~valid_g_3 && moving_forward_g) begin
			selected_g <= 0;
			moving_forward_g <=0;
		end
		if(~valid_o && ~valid_o_1 && ~valid_o_2 && ~valid_o_3 && moving_forward_o) begin 
			selected_o <= 0; 
			moving_forward_o <= 0;
		end 
		if(~valid_b && ~valid_b_1 && ~valid_b_2 && ~valid_b_3 && moving_forward_b) begin 
			selected_b <= 0; 
			moving_forward_b <= 0;
		end

        if(~valid_y && ~valid_y_1 && ~valid_y_2 && ~valid_y_3 && moving_forward_y)begin 
			selected_y <= 0; 
			moving_forward_y <= 0;
		end
        if(~valid_r && ~valid_c_1 && ~valid_c_2 && ~valid_c_3 && moving_forward_c)begin 
			selected_c <= 0; 
			moving_forward_c <= 0;
		end
        if(~valid_blue && ~valid_blue_1 && ~valid_blue_2 && ~valid_blue_3 && moving_forward_blue)begin 
			selected_blue <= 0; 
			moving_forward_blue <= 0;
		end
	end
	
end


always @(posedge clk)begin
	if(message_from_ESP32 == 70) begin
		data_colour <=  4'b1111;
	end
	else begin
        // TODO::NOT enough bits
		data_colour <=  (lock_r) ? 4'b000: 
						(lock_p) ? 4'b001: 
						(lock_g) ? 4'b010: 
						(lock_o) ? 4'b011: 
						(lock_b) ? 4'b100: 
                        (lock_y) ? 4'b101;
                        (lock_c) ? 4'b110;
                        (lock_blue) ? 4'b111
						(c_8 == distance_r && valid_r) ? 4'b000 :
						(c_8 == distance_p && valid_p) ? 4'b001 :
						(c_8 == distance_g && valid_g) ? 4'b010 :
						(c_8 == distance_o && valid_o) ? 4'b011 :
                        (c_8 == distance_b && valid_b) ? 4'b100 :
                        (c_8 == distance_y && valid_y) ? 4'b101 :
                        (c_8 == distance_c && valid_c) ? 4'b110 :
						(c_8 == distance_b && valid_blue) ? 4'b111 : 4'b1111;	
	end				 
end

always @(*) begin
	if(message_from_ESP32 == 10) begin
		//0
		message_to_ESP32 = {1'b0, 3'b000, 9'b0, ~valid_o, moving_forward_o, lock_o}; end 
	else if(message_from_ESP32 == 11)begin
		//1
		//message_to_ESP32 = {1'b0, 3'b001, 1'b0, y}; end
		message_to_ESP32 = {1'b0, 3'b001, 7'b0, valid_r, valid_p, valid_g, valid_o, valid_b}; end 
	else if(message_from_ESP32 == 12) begin
		//2
		message_to_ESP32 = {1'b0, 3'b010, 7'b0, selected_r, selected_p, selected_g, selected_o, selected_b}; end 	
	else if(message_from_ESP32 == 13) begin
		//3
		message_to_ESP32 = {1'b0, 3'b100, 7'b0, moving_forward_r, moving_forward_p, moving_forward_g, moving_forward_o, moving_forward_b};
	end
	else if(message_from_ESP32 == 14) begin
		//4
        // TODO:: not enough bits
		case(data_colour)
			0 : message_to_ESP32 = (formate_r)? {1'b0, data_colour, distance_r}: {1'b1, data_colour, red_center_x_pixel};	 
			//1 : message_to_ESP32 = (formate_p)? {1'b0, data_colour, distance_p}: {1'b1, data_colour, pink_center_x_pixel};
			//2 : message_to_ESP32 = (formate_g)? {1'b0, data_colour, distance_g}: {1'b1, data_colour, green_center_x_pixel};
			3 : message_to_ESP32 = (formate_o)? {1'b0, data_colour, distance_o}: {1'b1, data_colour, orange_center_x_pixel};
			4 : message_to_ESP32 = (formate_b)? {1'b0, data_colour, distance_b}: {1'b1, data_colour, black_center_x_pixel};
            5 : message_to_ESP32 = (formate_y)? {1'b0, data_colour, distance_y}: {1'b1, data_colour, yellow_center_x_pixel};
            6 : message_to_ESP32 = (formate_c)? {1'b0, data_colour, distance_c}: {1'b1, data_colour, cyan_center_x_pixel};
            7 : message_to_ESP32 = (formate_blue)? {1'b0, data_colour, distance_blue}: {1'b1, data_colour, blue_center_x_pixel};
			15 : message_to_ESP32 = 16'b1111111111111111;
			default : message_to_ESP32 = 16'b1111111111111111;
		endcase 
		end
	else if(message_from_ESP32 == 15) begin
		//5
		message_to_ESP32 = {1'b0, 3'b101, 1'b0, y_max_r}; end
	else if(message_from_ESP32 == 16) begin
		//6
		message_to_ESP32 = {1'b0, 3'b110,  1'b0, (avg_left_o + avg_right_o)/2}; end

	else if (message_from_ESP32 == 70) begin
 		if(~selected_o  && ~selected_r && ~selected_p && ~selected_g && ~selected_b) begin
			 message_to_ESP32 = 16'd60;
		 end else begin
			message_to_ESP32 = 16'd70;
		 end
		
	end 
	
	else begin
		 // TODO:: not enough bits
		case(data_colour)
			0 : message_to_ESP32 = (formate_r)? {1'b0, data_colour, distance_r}: {1'b1, data_colour, red_center_x_pixel};	 
			//1 : message_to_ESP32 = (formate_p)? {1'b0, data_colour, distance_p}: {1'b1, data_colour, pink_center_x_pixel};
			//2 : message_to_ESP32 = (formate_g)? {1'b0, data_colour, distance_g}: {1'b1, data_colour, green_center_x_pixel};
			3 : message_to_ESP32 = (formate_o)? {1'b0, data_colour, distance_o}: {1'b1, data_colour, orange_center_x_pixel};
			4 : message_to_ESP32 = (formate_b)? {1'b0, data_colour, distance_b}: {1'b1, data_colour, black_center_x_pixel};
            5 : message_to_ESP32 = (formate_y)? {1'b0, data_colour, distance_y}: {1'b1, data_colour, yellow_center_x_pixel};
            6 : message_to_ESP32 = (formate_c)? {1'b0, data_colour, distance_c}: {1'b1, data_colour, cyan_center_x_pixel};
            7 : message_to_ESP32 = (formate_blue)? {1'b0, data_colour, distance_blue}: {1'b1, data_colour, blue_center_x_pixel};
			15 : message_to_ESP32 = 16'b1111111111111111;
			default : message_to_ESP32 = 16'b1111111111111111;
		endcase 
	end
	//message_to_ESP32 = {distance_r, data_colour, valid_r,valid_b,valid_g,valid_p,valid_r, c_5};
end

reg lock_r, lock_p, lock_o, lock_g, lock_b;
always @(posedge clk) begin
	if(message_from_ESP32 == 70) begin
		lock_r <= 0;
		lock_p <= 0;
		lock_o <= 0;
		lock_g <= 0;
		lock_b <= 0;
        lock_c <= 0;
        lock_y <= 0;
        lock_blue <= 0;
	end 
	else begin
		case(data_colour)
			0 : begin
					if(selected_r)begin
						lock_r <= 0;
					end 
					else if(formate_r && detection_request) begin
						lock_r <=1;
					end
				end
			1 : begin
					if(selected_p)begin
						lock_p <= 0;
					end 
					else if(formate_p && detection_request) begin
						lock_p <=1;
					end
				end
			2 : begin
					if(selected_g)begin
						lock_g <= 0;
					end 
					else if(formate_g && detection_request) begin
						lock_g <=1;
					end
				end
			3 : begin
					if(selected_o)begin
						lock_o <= 0;
					end 
					else if(formate_o && detection_request) begin
						lock_o <=1;
					end
				end
			4 : begin
					if(selected_b)begin
						lock_b <= 0;
					end 
					else if(formate_b && detection_request) begin
						lock_b <=1;
					end
				end
            5 : begin
					if(selected_y)begin
						lock_y <= 0;
					end 
					else if(formate_y && detection_request) begin
						lock_y <=1;
					end
				end
            6 : begin
					if(selected_c)begin
						lock_c <= 0;
					end 
					else if(formate_c && detection_request) begin
						lock_c <=1;
					end
				end
            7 : begin
					if(selected_blue)begin
						lock_blue <= 0;
					end 
					else if(formate_blue && detection_request) begin
						lock_blue <=1;
					end
				end
		endcase
	end 
end
endmodule



module L_abs(
	input [7:0] L_in,
	output reg [7:0] L_out
);
	always @(*) begin
			if(2 * L_in > 255) begin
				L_out = 2 * L_in - 255;	
			end 
			else begin
				L_out = 255 - 2*L_in;
			end
		end
endmodule

module comparator(
    input [7:0] a_1,
    input [7:0] b_1,
    output reg [7:0] a_0,
    output reg [7:0] b_0
);

    always @(a_1, b_1) begin
        if (a_1 > b_1) begin
            a_0 = a_1;
            b_0 = b_1;
        end
        else begin
            a_0 = b_1;
            b_0 = a_1;
        end
    end
endmodule

module Median(
	input[7:0] reg_5,
	input [10:0] x_value,
	input [7:0] smooth_value,
	input [7:0] a,
	input [7:0] b,
	input [7:0] c,
	input [7:0] d,
	input [7:0] e,
	output [7:0] median
);

	parameter IMAGE_W = 11'd640;
	parameter IMAGE_H = 11'd480;
    wire [7:0] aa, bb, cc, dd, ee;
    wire [7:0] a1, b1, c1, d1, b2, c2, d2, e2, a2, b3, c3, d3, b4, c4, d4, e4, a5, b5, c5, d5;
    comparator c1l1( a,  b, a1, b1);
    comparator c2l1( c,  d, c1, d1);
    comparator c1l2(b1, c1, b2, c2);
    comparator c2l2(d1,  e, d2, e2);
    comparator c1l3(a1, b2, a2, b3);
    comparator c2l3(c2, d2, c3, d3);
    comparator c1l4(b3, c3, b4, c4);
    comparator c2l4(d3, e2, d4, e4);
    comparator c1l5(a2, b4, a5, b5);
    comparator c2l6(c4, d4, c5, d5);
	assign median = (x_value < 2)? smooth_value: (x_value % IMAGE_W > IMAGE_W - 2 )? reg_5 : c5; 

endmodule




