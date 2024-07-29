"""CSCI 455 - Embedded Systems: Robotics - Spring 2022"""

__author__ = "John Dolph"
__date__   = "3-25-22"

try:
    import serial
except:
    pass
import time, sys

class DeviceManager:

    #0 is dc motor fwd/bk MUST BE INITIALIZED TO 6000 less is forward
    #1 is dc motor turning
    #2 is waist
    #3 is head back and forth
    #4 is head tilt up down
    #5 is right arm top
    #6 is 2nd from top right arm
    #7 is 3rd from top right arm
    #8 is 4th from top right arm
    #9 is wrist right arm
    #10 is hand grip
    #11 is nothing?
    #12
    #13
    #14/e is 3rd from top left arm
    #15 is 4th from top left arm
    #16-17

    def __init__(self):

        self.WHEEL_MIN_MAX = [4500, 7500]
        self.TURN_MIN_MAX = [4000, 8000]
        self.WAIST_MIN_MAX = [3000, 9000]
        self.HEAD_MIN_MAX = [3000, 9000]
        self.LEFT_ARM_MIN_MAX = [3000, 9000]
        self.RIGHT_ARM_MIN_MAX = [3000, 9000]

        self.usb = None
        self.left_wheel_value = 5896
        self.right_wheel_value = 5896
        self.waist_value = 5896
        self.head_horizontal_value = 5896
        self.head_vertical_value = 5896
        self.turn_value = 5896
        self.left_arm_value = 5896
        self.right_arm_value = 5896
        #initialize usb serial port
        try:
            #serial port (usb)
            self.usb = serial.Serial('/dev/ttyACM0')
            print(self.usb.name)
            print(self.usb.baudrate)
        except:
            try:
                #serial port (usb)
                self.usb = serial.Serial('/dev/ttyACM1')
                print(self.usb.name)
                print(self.usb.baudrate)
            except:
                print("No servo serial ports found")
                # sys.exit(0)                

        self.reset_devices()

    def reset_devices(self):
        self.set_left_wheel()
        self.set_right_wheel()
        self.set_waist()
        self.set_head_horizontal()
        self.set_head_vertical()
        self.set_right_arm()

    def set_left_wheel(self, target_value=5896):

        # if self.WHEEL_MIN_MAX[0] < target_value < self.WHEEL_MIN_MAX[1]:
            print('moving forward')
            success = self.perform_command(0x00, target_value)

            # #update known value
            if success:
                self.left_wheel_value = target_value

    def set_right_wheel(self, target_value=5896):
        # if self.TURN_MIN_MAX[0] < target_value < self.TURN_MIN_MAX[1]:
            success = self.perform_command(0x01, target_value,True)

            #update known value
            if success:
                self.right_wheel_value = target_value
                self.turn_value = target_value

    def set_waist(self, target_value=5896):
        if self.WAIST_MIN_MAX[0] < target_value < self.WAIST_MIN_MAX[1]:
            success = self.perform_command(0x02, target_value)

            #update known value
            if success:
                self.waist_value = target_value

    def set_head_horizontal(self, target_value=5896):
        if self.HEAD_MIN_MAX[0] < target_value < self.HEAD_MIN_MAX[1]:
            success = self.perform_command(0x03, target_value)

            #update known value
            if success:
                self.head_horizontal_value = target_value

    def set_head_vertical(self, target_value=5896):
        if self.HEAD_MIN_MAX[0] < target_value < self.HEAD_MIN_MAX[1]:
            success = self.perform_command(0x04, target_value)

            #update known value
            if success:
                self.head_vertical_value = target_value

    def set_right_arm(self, target_value=5896):
        if self.RIGHT_ARM_MIN_MAX[0] < target_value < self.RIGHT_ARM_MIN_MAX[1]:
            success = self.perform_command(0X05, target_value)

            if success:
                self.right_wheel_value = target_value



    def perform_command(self, device_id, target_value, turn=False):
        """
        Attempts to perform a command
        """

        print(target_value)
        try:

            lsb = target_value &0x7F
            msb = (target_value >> 7) & 0x7F
            cmd =  chr(0xaa) + chr(0xC) + chr(0x04) + chr(device_id) + chr(lsb) + chr(msb)

            # print(cmd)

            if turn:

                target_one = 5896
                target_two = target_value
                lsb_one = target_one &0x7F
                msb_one = (target_one >> 7) & 0x7F
                lsb_two = target_two &0x7F
                msb_two = (target_two >> 7) & 0x7F

                cmd = chr(0xAA) + chr(0xc) + chr(0x1F) + chr(0x02) + chr(0x00) + chr(lsb_one) + chr(msb_one) + chr(lsb_two) + chr(msb_two)

            self.usb.write(cmd.encode('utf-8'))

            # while(target < 9000):
            #     target += 400
            #     lsb = target &0x7F
            #     msb = (target >> 7) & 0x7F
            #     cmd = chr(0xaa) + chr(0xC) + chr(0x04) + chr(0x00) + chr(lsb) + chr(msb)
            #     usb.write(cmd.encode('utf-8'))

        except:

            print('Command error...')

            return False

        return True
