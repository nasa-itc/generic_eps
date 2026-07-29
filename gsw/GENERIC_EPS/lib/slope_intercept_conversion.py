from openc3.conversions.conversion import Conversion

class SlopeInterceptConversion(Conversion):
    def __init__(self, packet_field, slope, intercept=0):
        super().__init__()
        self.converted_type = 'FLOAT'
        self.converted_bit_size = 32
        self.packet_field = packet_field
        self.slope = float(slope)
        self.intercept = float(intercept)

    def call(self, value, packet, buffer):
        # Slope
        value = self.slope * packet.read(self.packet_field)
        # Intercept
        value = value + self.intercept
        return value