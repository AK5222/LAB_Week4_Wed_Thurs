module top (
    input  logic spi_cs,
    input  logic spi_sck,
    input  logic spi_mosi,
    output logic [3:0] led
);

    localparam logic LED_ACTIVE_LOW = 1'b1;

    logic [7:0] shift_reg = 8'h00;
    logic [2:0] bit_count = 3'd0;
    logic [3:0] led_bits  = 4'h0;

    assign led = LED_ACTIVE_LOW ? ~led_bits : led_bits;

    always_ff @(posedge spi_sck or posedge spi_cs) begin
        if (spi_cs) begin
            // CS went high: reset for next transaction
            shift_reg <= 8'h00;
            bit_count <= 3'd0;
        end else begin
            // Sample MOSI on rising SCK edge (SPI Mode 0)
            shift_reg <= {shift_reg[6:0], spi_mosi};

            if (bit_count == 3'd7) begin
                // All 8 bits received — latch the bottom 4 into LEDs
                // The top 4 bits of fpga_data are always 0x0,
                // so the actual LED data sits in bits [3:0]
                led_bits <= {shift_reg[2:0], spi_mosi};
            end

            bit_count <= bit_count + 3'd1;
        end
    end

endmodule