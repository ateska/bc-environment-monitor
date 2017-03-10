#include <bc_common.h>
#include <bc_scheduler.h>
#include <bc_usb_cdc.h>
#include <bc_led.h>
#include <bc_button.h>
#include <bc_i2c.h>
#include <bc_tag_temperature.h>
#include <bc_tag_humidity.h>
#include <bc_tag_lux_meter.h>
#include <bc_tag_barometer.h>
#include <bc_lis2dh12.h>

#include <bc_module_co2.h>

static bc_scheduler_task_id_t task_id;
static bc_led_t led;
static bc_tag_temperature_t temperature_tag;
static bc_tag_humidity_t humidity_tag;
static bc_tag_lux_meter_t lux_meter;
static bc_tag_barometer_t barometer_tag;
static bc_lis2dh12_t lis2dh12;
static bc_module_co2_t co2_module;

struct
{
	struct { bool valid; float value; } temperature;
	struct { bool valid; float value; } humidity;
	struct { bool valid; float value; } luminosity;
	struct { bool valid; float value; } altitude;
	struct { bool valid; float value; } pressure;
	struct { bool valid; float value; } acceleration_x;
	struct { bool valid; float value; } acceleration_y;
	struct { bool valid; float value; } acceleration_z;

} i2c_sensors;


void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
	(void) event;
	(void) event_param;

	i2c_sensors.temperature.valid = bc_tag_temperature_get_temperature_celsius(self, &i2c_sensors.temperature.value);
}

void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
	(void) event;
	(void) event_param;

	i2c_sensors.humidity.valid = bc_tag_humidity_get_humidity_percentage(self, &i2c_sensors.humidity.value);
}

void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param)
{
	(void) event;
	(void) event_param;

	i2c_sensors.luminosity.valid = bc_tag_lux_meter_get_luminosity_lux(self, &i2c_sensors.luminosity.value);
}

void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param)
{
	(void) event;
	(void) event_param;

	i2c_sensors.altitude.valid = bc_tag_barometer_get_altitude_meter(self, &i2c_sensors.altitude.value);
	i2c_sensors.pressure.valid = bc_tag_barometer_get_pressure_pascal(self, &i2c_sensors.pressure.value);
}

void lis2dh12_event_handler(bc_lis2dh12_t *self, bc_lis2dh12_event_t event, void *event_param)
{
	(void) event_param;

	if (event == BC_LIS2DH12_EVENT_UPDATE)
	{
		bc_lis2dh12_result_g_t result;

		if (bc_lis2dh12_get_result_g(self, &result))
		{
			i2c_sensors.acceleration_x.value = result.x_axis;
			i2c_sensors.acceleration_y.value = result.y_axis;
			i2c_sensors.acceleration_z.value = result.z_axis;

			i2c_sensors.acceleration_x.valid = true;
			i2c_sensors.acceleration_y.valid = true;
			i2c_sensors.acceleration_z.valid = true;
		}
		else
		{
			i2c_sensors.acceleration_x.valid = false;
			i2c_sensors.acceleration_y.valid = false;
			i2c_sensors.acceleration_z.valid = false;
		}
	}
}


void button_event_handler(void *event_param)
{
	char buffer[100] = "---\r\n";
	bc_usb_cdc_write(buffer, strlen(buffer));

	if (i2c_sensors.temperature.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "t:%.04f\r\n", i2c_sensors.temperature.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}

	if (i2c_sensors.humidity.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "h:%.04f\r\n", i2c_sensors.humidity.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}

	if (i2c_sensors.luminosity.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "l:%.04f\r\n", i2c_sensors.luminosity.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}

	if (i2c_sensors.altitude.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "a:%.04f\r\n", i2c_sensors.altitude.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}

	if (i2c_sensors.pressure.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "p:%.04f\r\n", i2c_sensors.pressure.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}

	if (i2c_sensors.acceleration_x.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "ax:%.04f\r\n", i2c_sensors.acceleration_x.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}
	if (i2c_sensors.acceleration_y.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "ay:%.04f\r\n", i2c_sensors.acceleration_y.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}
	if (i2c_sensors.acceleration_z.valid)
	{
		snprintf(buffer, sizeof(buffer)-1, "az:%.04f\r\n", i2c_sensors.acceleration_z.value);
		bc_usb_cdc_write(buffer, strlen(buffer));
	}

	bc_led_pulse(&led, 10);

	strcpy(buffer,"===\r\n");
	bc_usb_cdc_write(buffer, strlen(buffer));

	bc_scheduler_plan_current_relative(5*1000);
}

void application_init(void)
{
	bc_usb_cdc_init();

	bc_led_init(&led, BC_GPIO_LED, false, false);
	bc_i2c_init(BC_I2C_I2C0, BC_I2C_SPEED_400_KHZ);
	bc_i2c_init(BC_I2C_I2C1, BC_I2C_SPEED_400_KHZ);

	bc_module_co2_init(&co2_module);

	bc_tag_temperature_init(&temperature_tag, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
	bc_tag_temperature_set_update_interval(&temperature_tag, 1000);
	bc_tag_temperature_set_event_handler(&temperature_tag, temperature_tag_event_handler, NULL);

	bc_tag_humidity_init(&humidity_tag, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
	bc_tag_humidity_set_update_interval(&humidity_tag, 1000);
	bc_tag_humidity_set_event_handler(&humidity_tag, humidity_tag_event_handler, NULL);
	
	bc_tag_lux_meter_init(&lux_meter, BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT);
	bc_tag_lux_meter_set_update_interval(&lux_meter, 1000);
	bc_tag_lux_meter_set_event_handler(&lux_meter, lux_meter_event_handler, NULL);

	bc_tag_barometer_init(&barometer_tag, BC_I2C_I2C0);
	bc_tag_barometer_set_update_interval(&barometer_tag, 1000);
	bc_tag_barometer_set_event_handler(&barometer_tag, barometer_tag_event_handler, NULL);

	bc_lis2dh12_init(&lis2dh12, BC_I2C_I2C0, 0x19);
	bc_lis2dh12_set_update_interval(&lis2dh12, 1000);
	bc_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);

	task_id = bc_scheduler_register(button_event_handler, NULL, 0);
}
