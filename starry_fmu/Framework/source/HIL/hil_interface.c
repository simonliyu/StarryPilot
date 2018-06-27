/*
 * File      : hil_interface.c
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-03-06     zoujiachi    first version.
 */
 
#include "global.h"
#include "hil_interface.h"
#include "uMCN.h"
#include "console.h"
#include "mavproxy.h"
#include "filter.h"
#include "ms5611.h"

MCN_DECLARE(HIL_STATE_Q);
MCN_DECLARE(HIL_SENSOR);
MCN_DECLARE(HIL_GPS);

static HIL_Option _hil_op;
static McnNode_t hil_state_node_t;
static McnNode_t hil_sensor_node_t;
static McnNode_t hil_gps_node_t;
static McnNode_t hil_baro_node_t;

static char* TAG = "HIL";

MCN_DECLARE(SENSOR_GYR);
MCN_DECLARE(SENSOR_FILTER_GYR);
MCN_DECLARE(SENSOR_ACC);
MCN_DECLARE(SENSOR_FILTER_ACC);
MCN_DECLARE(SENSOR_MAG);
MCN_DECLARE(SENSOR_FILTER_MAG);
MCN_DECLARE(SENSOR_BARO);

int hil_collect_data(void)
{
	static uint32_t last_time = 0;
	if(_hil_op == HIL_SENSOR_LEVEL){
		if(mcn_poll(hil_sensor_node_t)){
			uint32_t now = time_nowMs();
			//Console.print("%d\n", now - last_time);
			last_time = now;
			mavlink_hil_sensor_t hil_sensor;
			mcn_copy(MCN_ID(HIL_SENSOR), hil_sensor_node_t, &hil_sensor);
			//Console.print("acc:%f %f %f gyr:%f %f %f mag:%f %f %f\n", hil_sensor.xacc, hil_sensor.yacc, hil_sensor.zacc,
			//			hil_sensor.xgyro, hil_sensor.ygyro, hil_sensor.zgyro, hil_sensor.xmag, hil_sensor.ymag, hil_sensor.zmag);
			
			float gyr[3] = {hil_sensor.xgyro, hil_sensor.ygyro, hil_sensor.zgyro};
			float acc[3] = {hil_sensor.xacc, hil_sensor.yacc, hil_sensor.zacc};
			float mag[3] = {hil_sensor.xmag, hil_sensor.ymag, hil_sensor.zmag};
			
			MS5611_REPORT_Def hil_baro_report;
			hil_baro_report.temperature = hil_sensor.temperature;
			hil_baro_report.pressure = hil_sensor.abs_pressure;
			hil_baro_report.altitude = hil_sensor.pressure_alt;
			
			/* publish gyro data */
			gyrfilter_input(gyr);
			mcn_publish(MCN_ID(SENSOR_GYR), gyr);
			mcn_publish(MCN_ID(SENSOR_FILTER_GYR), gyrfilter_current());
			/* publish accel data */
			accfilter_input(acc);
			mcn_publish(MCN_ID(SENSOR_ACC), acc);
			mcn_publish(MCN_ID(SENSOR_FILTER_ACC), accfilter_current());
			/* publish mag data */
			magfilter_input(mag);
			mcn_publish(MCN_ID(SENSOR_MAG), mag);
			mcn_publish(MCN_ID(SENSOR_FILTER_MAG), magfilter_current());
			/* publish baro data */
			mcn_publish(MCN_ID(SENSOR_BARO), &hil_baro_report);
		}
	}else if(_hil_op == HIL_STATE_LEVEL){
		if(mcn_poll(hil_state_node_t)){
			mavlink_hil_state_quaternion_t hil_state_q;
			mcn_copy(MCN_ID(HIL_STATE_Q), hil_state_node_t, &hil_state_q);
			Console.print("q1:%.2f q2:%.2f q3:%.2f q4:%.2f rs:%.2f ps:%.2f ys:%.2f lat:%.2f lon:%.2f alt:%.2f vx:%.2f vy:%.2f vz:%.2f ax:%.2f ay:%.2f az:%.2f\n", 
				hil_state_q.attitude_quaternion[0],hil_state_q.attitude_quaternion[1],hil_state_q.attitude_quaternion[2],hil_state_q.attitude_quaternion[3],
				hil_state_q.rollspeed,hil_state_q.pitchspeed,hil_state_q.yawspeed,hil_state_q.lat,hil_state_q.lon,hil_state_q.alt,
				hil_state_q.vx,hil_state_q.vy,hil_state_q.vz,hil_state_q.xacc,hil_state_q.yacc,hil_state_q.zacc);
		}
	}		
		
	return 0;
}

int hil_actuator_control(float *control, int motor_num)
{
	
}

bool hil_baro_poll(void)
{
	return mcn_poll(hil_baro_node_t);
}

int hil_interface_init(HIL_Option hil_op)
{
	_hil_op = hil_op;
	
	if(_hil_op == HIL_SENSOR_LEVEL){
		hil_sensor_node_t = mcn_subscribe(MCN_ID(HIL_SENSOR), NULL);
		if(hil_sensor_node_t == NULL){
			Console.e(TAG, "hil_sensor_node_t subscribe err\n");
		}
		
		hil_gps_node_t = mcn_subscribe(MCN_ID(HIL_GPS), NULL);
		if(hil_gps_node_t == NULL){
			Console.e(TAG, "hil_gps_node_t subscribe err\n");
		}
		
		hil_baro_node_t = mcn_subscribe(MCN_ID(SENSOR_BARO), NULL);
		if(hil_baro_node_t == NULL){
			Console.e(TAG, "hil_baro_node_t subscribe err\n");
		}
	}else if(_hil_op == HIL_STATE_LEVEL){
		hil_state_node_t = mcn_subscribe(MCN_ID(HIL_STATE_Q), NULL);
		if(hil_state_node_t == NULL){
			Console.e(TAG, "hil_state_node_t subscribe err\n");
		}
	}else{
		Console.e(TAG, "err, unknow hil_op\n");
		return 1;
	}
	
	return 0;
}
