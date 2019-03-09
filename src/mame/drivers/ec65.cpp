// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Robbbert
/***************************************************************************

EC-65 (also known as Octopus)

2009-07-16 Initial driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "cpu/g65816/g65816.h"
#include "video/mc6845.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "machine/mos6551.h"
#include "machine/6850acia.h"
#include "machine/keyboard.h"
#include "bus/rs232/rs232.h"
#include "emupal.h"
#include "screen.h"

#define PIA6821_TAG "pia6821"
#define ACIA6850_TAG "console"
#define ACIA6551_TAG "aux"
#define VIA6522_0_TAG "via6522_0"
#define VIA6522_1_TAG "via6522_1"
#define MC6845_TAG "mc6845"
#define KEYBOARD_TAG "keyboard"

class ec65_state : public driver_device
{
public:
	ec65_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_via_0(*this, VIA6522_0_TAG)
		, m_via_1(*this, VIA6522_1_TAG)
		, m_p_videoram(*this, "videoram")
		, m_p_chargen(*this, "chargen")
		, m_maincpu(*this, "maincpu")
		, m_palette(*this, "palette")
	{
	}

	void kbd_put(u8 data);
	MC6845_UPDATE_ROW(crtc_update_row);

	void ec65(machine_config &config);
	void ec65_mem(address_map &map);
private:
	virtual void machine_reset() override;
	required_device<via6522_device> m_via_0;
	required_device<via6522_device> m_via_1;
	required_shared_ptr<uint8_t> m_p_videoram;
	required_region_ptr<u8> m_p_chargen;
	required_device<cpu_device> m_maincpu;
	required_device<palette_device> m_palette;
};

class ec65k_state : public driver_device
{
public:
	ec65k_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
	{
	}
	void ec65k(machine_config &config);
	void ec65k_mem(address_map &map);
};

void ec65_state::ec65_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0xdfff).ram();
	map(0xe000, 0xe003).rw(PIA6821_TAG, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0xe100, 0xe10f).rw(m_via_0, FUNC(via6522_device::read), FUNC(via6522_device::write));
	map(0xe110, 0xe11f).rw(m_via_1, FUNC(via6522_device::read), FUNC(via6522_device::write));
	map(0xe130, 0xe133).rw(ACIA6551_TAG, FUNC(mos6551_device::read), FUNC(mos6551_device::write));
	map(0xe140, 0xe140).w(MC6845_TAG, FUNC(mc6845_device::address_w));
	map(0xe141, 0xe141).rw(MC6845_TAG, FUNC(mc6845_device::register_r), FUNC(mc6845_device::register_w));
	map(0xe150, 0xe153).rw(ACIA6850_TAG, FUNC(acia6850_device::read), FUNC(acia6850_device::write));
	map(0xe400, 0xe7ff).ram(); // 1KB on-board RAM
	map(0xe800, 0xefff).ram().share("videoram");
	map(0xf000, 0xffff).rom();
}

void ec65k_state::ec65k_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0xe7ff).ram();
	map(0xe800, 0xefff).ram().share("videoram");
	map(0xf000, 0xffff).rom();
}

/* Input ports */
static INPUT_PORTS_START( ec65 )
INPUT_PORTS_END

void ec65_state::kbd_put(u8 data)
{
	if (data)
	{
		m_via_0->write_pa0(BIT(data, 0));
		m_via_0->write_pa1(BIT(data, 1));
		m_via_0->write_pa2(BIT(data, 2));
		m_via_0->write_pa3(BIT(data, 3));
		m_via_0->write_pa4(BIT(data, 4));
		m_via_0->write_pa5(BIT(data, 5));
		m_via_0->write_pa6(BIT(data, 6));
		m_via_0->write_pa7(BIT(data, 7));
		m_via_0->write_ca1(1);
		m_via_0->write_ca1(0);
	}
}

void ec65_state::machine_reset()
{
	m_via_1->write_pb0(1);
	m_via_1->write_pb1(1);
	m_via_1->write_pb2(1);
	m_via_1->write_pb3(1);
	m_via_1->write_pb4(1);
	m_via_1->write_pb5(1);
	m_via_1->write_pb6(1);
	m_via_1->write_pb7(1);
}

MC6845_UPDATE_ROW( ec65_state::crtc_update_row )
{
	const rgb_t *palette = m_palette->palette()->entry_list_raw();
	uint8_t chr,gfx,inv;
	uint16_t mem,x;
	uint32_t *p = &bitmap.pix32(y);

	for (x = 0; x < x_count; x++)
	{
		inv = (x == cursor_x) ? 0xff : 0;
		mem = (ma + x) & 0x7ff;
		chr = m_p_videoram[mem];

		/* get pattern of pixels for that character scanline */
		gfx = m_p_chargen[(chr<<4) | (ra & 0x0f)] ^ inv;

		/* Display a scanline of a character */
		*p++ = palette[BIT(gfx, 7)];
		*p++ = palette[BIT(gfx, 6)];
		*p++ = palette[BIT(gfx, 5)];
		*p++ = palette[BIT(gfx, 4)];
		*p++ = palette[BIT(gfx, 3)];
		*p++ = palette[BIT(gfx, 2)];
		*p++ = palette[BIT(gfx, 1)];
		*p++ = palette[BIT(gfx, 0)];
	}
}

/* F4 Character Displayer */
static const gfx_layout ec65_charlayout =
{
	8, 8,                   /* 8 x 8 characters */
	256,                    /* 256 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*16                    /* every char takes 16 bytes */
};

static GFXDECODE_START( gfx_ec65 )
	GFXDECODE_ENTRY( "chargen", 0x0000, ec65_charlayout, 0, 1 )
GFXDECODE_END

MACHINE_CONFIG_START(ec65_state::ec65)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu",M6502, XTAL(4'000'000) / 4)
	MCFG_DEVICE_PROGRAM_MAP(ec65_mem)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MCFG_SCREEN_UPDATE_DEVICE(MC6845_TAG, mc6845_device, screen_update)

	GFXDECODE(config, "gfxdecode", m_palette, gfx_ec65);
	PALETTE(config, "palette", palette_device::MONOCHROME);

	mc6845_device &crtc(MC6845(config, MC6845_TAG, XTAL(16'000'000) / 8));
	crtc.set_screen("screen");
	crtc.set_show_border_area(false);
	crtc.set_char_width(8); /*?*/
	crtc.set_update_row_callback(FUNC(ec65_state::crtc_update_row), this);

	/* devices */
	PIA6821(config, PIA6821_TAG, 0);

	//ACIA6850(config, ACIA6850_TAG, 0);
	acia6850_device &console(ACIA6850(config, ACIA6850_TAG, 0));
	console.txd_handler().set("eia0", FUNC(rs232_port_device::write_txd));
	console.rts_handler().set("eia0", FUNC(rs232_port_device::write_rts));

	clock_device &aclock(CLOCK(config, "aclock", 153600));
	aclock.signal_handler().set("console", FUNC(acia6850_device::write_txc));
	aclock.signal_handler().append("console", FUNC(acia6850_device::write_rxc));

	rs232_port_device &eia0(RS232_PORT(config, "eia0", default_rs232_devices, "terminal"));
	eia0.rxd_handler().set("console", FUNC(acia6850_device::write_rxd));
	eia0.dcd_handler().set("console", FUNC(acia6850_device::write_dcd));
	//eia0.dsr_handler().set("console", FUNC(acia6850_device::write_dsr));
	eia0.cts_handler().set("console", FUNC(acia6850_device::write_cts));
	//eia0.rxd_handler().set("aux", FUNC(mos6551_device::write_rxd));
	//eia0.dcd_handler().set("aux", FUNC(mos6551_device::write_dcd));
	//eia0.dsr_handler().set("aux", FUNC(mos6551_device::write_dsr));
	//eia0.cts_handler().set("aux", FUNC(mos6551_device::write_cts));

	VIA6522(config, m_via_0, XTAL(4'000'000) / 4);

	VIA6522(config, m_via_1, XTAL(4'000'000) / 4);

	mos6551_device &acia(MOS6551(config, ACIA6551_TAG, 0));
	acia.set_xtal(XTAL(1'843'200));
	//acia.txd_handler().set("eia0", FUNC(rs232_port_device::write_txd));
	//acia.rts_handler().set("eia0", FUNC(rs232_port_device::write_rts));
	//acia.dtr_handler().set("eia0", FUNC(rs232_port_device::write_dtr));

	generic_keyboard_device &keyboard(GENERIC_KEYBOARD(config, KEYBOARD_TAG, 0));
	keyboard.set_keyboard_callback(FUNC(ec65_state::kbd_put));
MACHINE_CONFIG_END

MACHINE_CONFIG_START(ec65k_state::ec65k)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu",G65816, XTAL(4'000'000)) // can use 4,2 or 1 MHz
	MCFG_DEVICE_PROGRAM_MAP(ec65k_mem)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MCFG_SCREEN_UPDATE_DEVICE(MC6845_TAG, mc6845_device, screen_update)

	GFXDECODE(config, "gfxdecode", "palette", gfx_ec65);
	PALETTE(config, "palette", palette_device::MONOCHROME);

	mc6845_device &crtc(MC6845(config, MC6845_TAG, XTAL(16'000'000) / 8));
	crtc.set_screen("screen");
	crtc.set_show_border_area(false);
	crtc.set_char_width(8); /*?*/
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ec65 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ec65.ic6", 0xf000, 0x1000, CRC(acd928ed) SHA1(e02a688a057ff77294717cf7b887425fed0b1153))

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen.ic19", 0x0000, 0x1000, CRC(9b56a28d) SHA1(41c04fd9fb542c50287bc0e366358a61fc4b0cd4)) // Located on VDU card
ROM_END

ROM_START( ec65k )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ec65k.ic19",  0xf000, 0x1000, CRC(5e5a890a) SHA1(daa006f2179fd156833e11c73b37881cafe5dede))

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen.ic19", 0x0000, 0x1000, CRC(9b56a28d) SHA1(41c04fd9fb542c50287bc0e366358a61fc4b0cd4)) // Located on VDU card, suspect bad dump
ROM_END
/* Driver */

/*    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  CLASS        INIT        COMPANY                FULLNAME  FLAGS */
COMP( 1985, ec65,  0,      0,      ec65,    ec65,  ec65_state,  empty_init, "Elektor Electronics", "EC-65",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW)
COMP( 1985, ec65k, ec65,   0,      ec65k,   ec65,  ec65k_state, empty_init, "Elektor Electronics", "EC-65K", MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW)
