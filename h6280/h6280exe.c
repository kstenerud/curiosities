/* Beginning of execute function */
#define EXECUTE_START																		\
int h6280_execute(int clocks)																\
{																							\
	if(!LINE_RST)																			\
	{																						\
		CLOCKS = clocks;																	\
		TIQ_POINT = CLOCKS - REG_TIMER_VALUE;			/* Point to generate timer int */	\
		do																					\
		{																					\
			REG_PPC = REG_PC;																\
			H6280_CALL_DEBUGGER;															\
			REG_PC++;																		\
			REG_IR = read_8_instruction(REG_PPC);



/* End of execute function */
#define EXECUTE_END																			\
			if(REG_TIMER_ENABLE && CLOCKS < TIQ_POINT)	/* timer generated carry */			\
			{																				\
				TIQ_POINT -= REG_TIMER_PERIOD;			/* reset timer & handle overlap */	\
				if(TIQ_POINT > CLOCKS)					/* Keep it sane */					\
					TIQ_POINT = CLOCKS;														\
				if(!(REG_INT_STATES & INT_TIQ))			/* ignore if TIQ already set */		\
				{																			\
					REG_INT_STATES |= INT_TIQ;			/* set TIQ "line" */				\
					PENDING_INTS |= INT_TIQ;			/* add TIQ to pending list */		\
				}																			\
			}																				\
		} while(CLOCKS > 0);																\
		REG_TIMER_VALUE = CLOCKS - TIQ_POINT;												\
		return clocks - CLOCKS;																\
	}																						\
	return clocks;																			\
}


/* Allow building of the core using a big switch statement or a jump table.
 * Big switch can give better performance on some systems but may give some
 * compilers trouble.
 */
#if H6280_BIG_SWITCH

#define OP(CODE, CLOCKS, OPERATION) case 0x##CODE: CLK(CLOCKS); ##OPERATION; break;
#define OPCODES_START  EXECUTE_START switch(REG_IR)	{
#define OPCODES_END    } EXECUTE_END

#else /* H6280_BIG_SWITCH */

#define OP(CODE, CLOCKS, OPERATION) static void h6280i_##CODE(void) {CLK(CLOCKS); ##OPERATION;}
#define O(NUM) h6280i_##NUM
#define OPCODES_START
#define OPCODES_END                                  \
static void (*h6280i_opcodes[256])(void) =           \
{                                                    \
	O(00),O(01),O(02),O(03),O(04),O(05),O(06),O(07), \
	O(08),O(09),O(0a),O(0b),O(0c),O(0d),O(0e),O(0f), \
	O(10),O(11),O(12),O(13),O(14),O(15),O(16),O(17), \
	O(18),O(19),O(1a),O(1b),O(1c),O(1d),O(1e),O(1f), \
	O(20),O(21),O(22),O(23),O(24),O(25),O(26),O(27), \
	O(28),O(29),O(2a),O(2b),O(2c),O(2d),O(2e),O(2f), \
	O(30),O(31),O(32),O(33),O(34),O(35),O(36),O(37), \
	O(38),O(39),O(3a),O(3b),O(3c),O(3d),O(3e),O(3f), \
	O(40),O(41),O(42),O(43),O(44),O(45),O(46),O(47), \
	O(48),O(49),O(4a),O(4b),O(4c),O(4d),O(4e),O(4f), \
	O(50),O(51),O(52),O(53),O(54),O(55),O(56),O(57), \
	O(58),O(59),O(5a),O(5b),O(5c),O(5d),O(5e),O(5f), \
	O(60),O(61),O(62),O(63),O(64),O(65),O(66),O(67), \
	O(68),O(69),O(6a),O(6b),O(6c),O(6d),O(6e),O(6f), \
	O(70),O(71),O(72),O(73),O(74),O(75),O(76),O(77), \
	O(78),O(79),O(7a),O(7b),O(7c),O(7d),O(7e),O(7f), \
	O(80),O(81),O(82),O(83),O(84),O(85),O(86),O(87), \
	O(88),O(89),O(8a),O(8b),O(8c),O(8d),O(8e),O(8f), \
	O(90),O(91),O(92),O(93),O(94),O(95),O(96),O(97), \
	O(98),O(99),O(9a),O(9b),O(9c),O(9d),O(9e),O(9f), \
	O(a0),O(a1),O(a2),O(a3),O(a4),O(a5),O(a6),O(a7), \
	O(a8),O(a9),O(aa),O(ab),O(ac),O(ad),O(ae),O(af), \
	O(b0),O(b1),O(b2),O(b3),O(b4),O(b5),O(b6),O(b7), \
	O(b8),O(b9),O(ba),O(bb),O(bc),O(bd),O(be),O(bf), \
	O(c0),O(c1),O(c2),O(c3),O(c4),O(c5),O(c6),O(c7), \
	O(c8),O(c9),O(ca),O(cb),O(cc),O(cd),O(ce),O(cf), \
	O(d0),O(d1),O(d2),O(d3),O(d4),O(d5),O(d6),O(d7), \
	O(d8),O(d9),O(da),O(db),O(dc),O(dd),O(de),O(df), \
	O(e0),O(e1),O(e2),O(e3),O(e4),O(e5),O(e6),O(e7), \
	O(e8),O(e9),O(ea),O(eb),O(ec),O(ed),O(ee),O(ef), \
	O(f0),O(f1),O(f2),O(f3),O(f4),O(f5),O(f6),O(f7), \
	O(f8),O(f9),O(fa),O(fb),O(fc),O(fd),O(fe),O(ff)  \
};                                                   \
EXECUTE_START                                        \
	h6280i_opcodes[REG_IR]();                        \
EXECUTE_END

#endif /* H6280_BIG_SWITCH */



/* Build the opcodes and the execute function */

OPCODES_START
/* C = M65C02 opcode
 * H = HuC6280 opcode
 * blank = M6502 opcode
 */
/*  OP CLK FUNCTION                       Comment     */
OP( 00, 8, OP_BRK   (              ) ) /* BRK         */
OP( 01, 7, OP_ORA   ( IDX          ) ) /* ORA idx     */
OP( 02, 3, OP_SWAP  ( REG_X, REG_Y ) ) /* SXY     (H) */
OP( 03, 4, OP_ST0   (              ) ) /* ST0     (H) */
OP( 04, 6, OP_TSB   ( ZPG          ) ) /* TSB zp  (C) */
OP( 05, 4, OP_ORA   ( ZPG          ) ) /* ORA zp      */
OP( 06, 6, OP_ASL_M ( ZPG          ) ) /* ASL zp      */
OP( 07, 7, OP_RMB   ( ZPG, BIT_0   ) ) /* RMB0    (C) */
OP( 08, 3, OP_PHP   (              ) ) /* PHP         */
OP( 09, 2, OP_ORA   ( IMM          ) ) /* ORA imm     */
OP( 0a, 2, OP_ASL_A (              ) ) /* ASL acc     */
OP( 0b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 0c, 7, OP_TSB   ( ABS          ) ) /* TSB abs (C) */
OP( 0d, 5, OP_ORA   ( ABS          ) ) /* ORA abs     */
OP( 0e, 7, OP_ASL_M ( ABS          ) ) /* ASL abs     */
OP( 0f, 0, OP_BBR   ( BIT_0        ) ) /* BBR0    (C) */
OP( 10, 0, OP_BCC   ( COND_PL()    ) ) /* BPL         */
OP( 11, 7, OP_ORA   ( IDY          ) ) /* ORA idy     */
OP( 12, 7, OP_ORA   ( ZPI          ) ) /* ORA zpi (C) */
OP( 13, 4, OP_ST1   (              ) ) /* ST1     (H) */
OP( 14, 6, OP_TRB   ( ZPG          ) ) /* TRB zp  (C) */
OP( 15, 4, OP_ORA   ( ZPX          ) ) /* ORA zpx     */
OP( 16, 6, OP_ASL_M ( ZPX          ) ) /* ASL zpx     */
OP( 17, 7, OP_RMB   ( ZPG, BIT_1   ) ) /* RMB1    (C) */
OP( 18, 2, OP_CLC   (              ) ) /* CLC         */
OP( 19, 5, OP_ORA   ( ABY          ) ) /* ORA aby     */
OP( 1a, 2, OP_INC_R ( REG_A        ) ) /* INA     (C) */
OP( 1b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 1c, 7, OP_TRB   ( ABS          ) ) /* TRB abs (C) */
OP( 1d, 5, OP_ORA   ( ABX          ) ) /* ORA abx     */
OP( 1e, 7, OP_ASL_M ( ABX          ) ) /* ASL abx     */
OP( 1f, 0, OP_BBR   ( BIT_1        ) ) /* BBR1    (C) */
OP( 20, 7, OP_JSR   ( ABS          ) ) /* JSR         */
OP( 21, 7, OP_AND   ( IDX          ) ) /* AND idx     */
OP( 22, 3, OP_SWAP  ( REG_A, REG_X ) ) /* SAX     (H) */
OP( 23, 4, OP_ST2   (              ) ) /* ST2     (H) */
OP( 24, 4, OP_BIT   ( ZPG          ) ) /* BIT zp      */
OP( 25, 4, OP_AND   ( ZPG          ) ) /* AND zp      */
OP( 26, 6, OP_ROL_M ( ZPG          ) ) /* ROL zp      */
OP( 27, 7, OP_RMB   ( ZPG, BIT_2   ) ) /* RMB2    (C) */
OP( 28, 4, OP_PLP   (              ) ) /* PLP         */
OP( 29, 2, OP_AND   ( IMM          ) ) /* AND imm     */
OP( 2a, 2, OP_ROL_A (              ) ) /* ROL acc     */
OP( 2b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 2c, 5, OP_BIT   ( ABS          ) ) /* BIT abs     */
OP( 2d, 5, OP_AND   ( ABS          ) ) /* AND abs     */
OP( 2e, 7, OP_ROL_M ( ABS          ) ) /* ROL abs     */
OP( 2f, 0, OP_BBR   ( BIT_2        ) ) /* BBR2    (C) */
OP( 30, 0, OP_BCC   ( COND_MI()    ) ) /* BMI         */
OP( 31, 7, OP_AND   ( IDY          ) ) /* AND idy     */
OP( 32, 7, OP_AND   ( ZPI          ) ) /* AND zpi (C) */
OP( 33, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 34, 4, OP_BIT   ( ZPX          ) ) /* BIT zpx (C) */
OP( 35, 4, OP_AND   ( ZPX          ) ) /* AND zpx     */
OP( 36, 6, OP_ROL_M ( ZPX          ) ) /* ROL zpx     */
OP( 37, 7, OP_RMB   ( ZPG, BIT_3   ) ) /* RMB3    (C) */
OP( 38, 2, OP_SEC   (              ) ) /* SEC         */
OP( 39, 5, OP_AND   ( ABY          ) ) /* AND aby     */
OP( 3a, 2, OP_DEC_R ( REG_A        ) ) /* DEA     (C) */
OP( 3b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 3c, 5, OP_BIT   ( ABX          ) ) /* BIT abx (C) */
OP( 3d, 5, OP_AND   ( ABX          ) ) /* AND abx     */
OP( 3e, 7, OP_ROL_M ( ABX          ) ) /* ROL abx     */
OP( 3f, 0, OP_BBR   ( BIT_3        ) ) /* BBR3    (C) */
OP( 40, 7, OP_RTI   (              ) ) /* RTI         */
OP( 41, 7, OP_EOR   ( IDX          ) ) /* EOR idx     */
OP( 42, 3, OP_SWAP  ( REG_A, REG_Y ) ) /* SAY     (H) */
OP( 43, 4, OP_TMA   (              ) ) /* TMA     (H) */
OP( 44, 8, OP_BSR   (              ) ) /* BSR     (H) */
OP( 45, 4, OP_EOR   ( ZPG          ) ) /* EOR zp      */
OP( 46, 6, OP_LSR_M ( ZPG          ) ) /* LSR zp      */
OP( 47, 7, OP_RMB   ( ZPG, BIT_4   ) ) /* RMB4    (C) */
OP( 48, 3, OP_PUSH  ( REG_A        ) ) /* PHA         */
OP( 49, 2, OP_EOR   ( IMM          ) ) /* EOR imm     */
OP( 4a, 2, OP_LSR_A (              ) ) /* LSR acc     */
OP( 4b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 4c, 4, OP_JMP   ( ABS          ) ) /* JMP abs     */
OP( 4d, 5, OP_EOR   ( ABS          ) ) /* EOR abs     */
OP( 4e, 7, OP_LSR_M ( ABS          ) ) /* LSR abs     */
OP( 4f, 0, OP_BBR   ( BIT_4        ) ) /* BBR4    (C) */
OP( 50, 0, OP_BCC   ( COND_VC()    ) ) /* BVC         */
OP( 51, 7, OP_EOR   ( IDY          ) ) /* EOR idy     */
OP( 52, 7, OP_EOR   ( ZPI          ) ) /* EOR zpi (C) */
OP( 53, 5, OP_TAM   (              ) ) /* TAM     (H) */
OP( 54, 2, OP_CSL   (              ) ) /* CSL     (H) */
OP( 55, 4, OP_EOR   ( ZPX          ) ) /* EOR zpx     */
OP( 56, 6, OP_LSR_M ( ZPX          ) ) /* LSR zpx     */
OP( 57, 7, OP_RMB   ( ZPG, BIT_5   ) ) /* RMB5    (C) */
OP( 58, 2, OP_CLI   (              ) ) /* CLI         */
OP( 59, 5, OP_EOR   ( ABY          ) ) /* EOR aby     */
OP( 5a, 3, OP_PUSH  ( REG_Y        ) ) /* PHY     (C) */
OP( 5b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 5c, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 5d, 5, OP_EOR   ( ABX          ) ) /* EOR abx     */
OP( 5e, 7, OP_LSR_M ( ABX          ) ) /* LSR abx     */
OP( 5f, 0, OP_BBR   ( BIT_5        ) ) /* BBR5    (C) */
OP( 60, 7, OP_RTS   (              ) ) /* RTS         */
OP( 61, 7, OP_ADC   ( IDX          ) ) /* ADC idx     */
OP( 62, 2, OP_CLA   (              ) ) /* CLA     (H) */
OP( 63, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 64, 4, OP_ST    ( 0, ZPG       ) ) /* STZ zp  (C) */
OP( 65, 4, OP_ADC   ( ZPG          ) ) /* ADC zp      */
OP( 66, 6, OP_ROR_M ( ZPG          ) ) /* ROR zp      */
OP( 67, 7, OP_RMB   ( ZPG, BIT_6   ) ) /* RMB6    (C) */
OP( 68, 4, OP_PLA   (              ) ) /* PLA         */
OP( 69, 2, OP_ADC   ( IMM          ) ) /* ADC imm     */
OP( 6a, 2, OP_ROR_A (              ) ) /* ROR acc     */
OP( 6b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 6c, 7, OP_JMP   ( IND          ) ) /* JMP ind     */
OP( 6d, 5, OP_ADC   ( ABS          ) ) /* ADC abs     */
OP( 6e, 7, OP_ROR_M ( ABS          ) ) /* ROR abs     */
OP( 6f, 0, OP_BBR   ( BIT_6        ) ) /* BBR6    (C) */
OP( 70, 0, OP_BCC   ( COND_VS()    ) ) /* BVS         */
OP( 71, 7, OP_ADC   ( IDY          ) ) /* ADC idy     */
OP( 72, 7, OP_ADC   ( ZPI          ) ) /* ADC zpi (C) */
OP( 73, 0, OP_TII   (              ) ) /* TII     (H) */
OP( 74, 4, OP_ST    ( 0, ZPX       ) ) /* STZ zpx (C) */
OP( 75, 4, OP_ADC   ( ZPX          ) ) /* ADC zpx     */
OP( 76, 6, OP_ROR_M ( ZPX          ) ) /* ROR zpx     */
OP( 77, 7, OP_RMB   ( ZPG, BIT_7   ) ) /* RMB7    (C) */
OP( 78, 2, OP_SEI   (              ) ) /* SEI         */
OP( 79, 5, OP_ADC   ( ABY          ) ) /* ADC aby     */
OP( 7a, 4, OP_PULL  ( REG_Y        ) ) /* PLY     (C) */
OP( 7b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 7c, 7, OP_JMP   ( IAX          ) ) /* JMP iax (C) */
OP( 7d, 5, OP_ADC   ( ABX          ) ) /* ADC abx     */
OP( 7e, 7, OP_ROR_M ( ABX          ) ) /* ROR abx     */
OP( 7f, 0, OP_BBR   ( BIT_7        ) ) /* BBR7    (C) */
OP( 80, 4, OP_BRA   (              ) ) /* BRA     (C) */
OP( 81, 7, OP_ST    ( REG_A, IDX   ) ) /* STA idx     */
OP( 82, 2, OP_CLX   (              ) ) /* CLX     (H) */
OP( 83, 7, OP_TST   ( ZPG          ) ) /* TST zpg (H) */
OP( 84, 4, OP_ST    ( REG_Y, ZPG   ) ) /* STY zp      */
OP( 85, 4, OP_ST    ( REG_A, ZPG   ) ) /* STA zp      */
OP( 86, 4, OP_ST    ( REG_X, ZPG   ) ) /* STX zp      */
OP( 87, 7, OP_SMB   ( ZPG, BIT_0   ) ) /* SMB0    (C) */
OP( 88, 2, OP_DEC_R ( REG_Y        ) ) /* DEY         */
OP( 89, 2, OP_BIT   ( IMM          ) ) /* BIT imm (C) */
OP( 8a, 2, OP_TRANS ( REG_X, REG_A ) ) /* TXA         */
OP( 8b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 8c, 5, OP_ST    ( REG_Y, ABS   ) ) /* STY abs     */
OP( 8d, 5, OP_ST    ( REG_A, ABS   ) ) /* STA abs     */
OP( 8e, 5, OP_ST    ( REG_X, ABS   ) ) /* STX abs     */
OP( 8f, 0, OP_BBS   ( BIT_0        ) ) /* BBS0    (C) */
OP( 90, 0, OP_BCC   ( COND_CC()    ) ) /* BCC         */
OP( 91, 7, OP_ST    ( REG_A, IDY   ) ) /* STA idy     */
OP( 92, 7, OP_ST    ( REG_A, ZPI   ) ) /* STA zpi (C) */
OP( 93, 8, OP_TST   ( ABS          ) ) /* TST abs (H) */
OP( 94, 4, OP_ST    ( REG_Y, ZPX   ) ) /* STY zpx     */
OP( 95, 4, OP_ST    ( REG_A, ZPX   ) ) /* STA zpx     */
OP( 96, 4, OP_ST    ( REG_X, ZPY   ) ) /* STX zpy     */
OP( 97, 7, OP_SMB   ( ZPG, BIT_1   ) ) /* SMB1    (C) */
OP( 98, 2, OP_TRANS ( REG_Y, REG_A ) ) /* TYA         */
OP( 99, 5, OP_ST    ( REG_A, ABY   ) ) /* STA aby     */
OP( 9a, 2, OP_TXS   (              ) ) /* TXS         */
OP( 9b, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( 9c, 5, OP_ST    ( 0, ABS       ) ) /* STZ abs (C) */
OP( 9d, 5, OP_ST    ( REG_A, ABX   ) ) /* STA abx     */
OP( 9e, 5, OP_ST    ( 0, ABX       ) ) /* STZ abx (C) */
OP( 9f, 0, OP_BBS   ( BIT_1        ) ) /* BBS1    (C) */
OP( a0, 2, OP_LD    ( REG_Y, IMM   ) ) /* LDY imm     */
OP( a1, 7, OP_LD    ( REG_A, IDX   ) ) /* LDA idx     */
OP( a2, 2, OP_LD    ( REG_X, IMM   ) ) /* LDX imm     */
OP( a3, 7, OP_TST   ( ZPX          ) ) /* TST zpx (H) */
OP( a4, 4, OP_LD    ( REG_Y, ZPG   ) ) /* LDY zp      */
OP( a5, 4, OP_LD    ( REG_A, ZPG   ) ) /* LDA zp      */
OP( a6, 4, OP_LD    ( REG_X, ZPG   ) ) /* LDX zp      */
OP( a7, 7, OP_SMB   ( ZPG, BIT_2   ) ) /* SMB2    (C) */
OP( a8, 2, OP_TRANS ( REG_A, REG_Y ) ) /* TAY         */
OP( a9, 2, OP_LD    ( REG_A, IMM   ) ) /* LDA imm     */
OP( aa, 2, OP_TRANS ( REG_A, REG_X ) ) /* TAX         */
OP( ab, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( ac, 5, OP_LD    ( REG_Y, ABS   ) ) /* LDY abs     */
OP( ad, 5, OP_LD    ( REG_A, ABS   ) ) /* LDA abs     */
OP( ae, 5, OP_LD    ( REG_X, ABS   ) ) /* LDX abs     */
OP( af, 0, OP_BBS   ( BIT_2        ) ) /* BBS2    (C) */
OP( b0, 0, OP_BCC   ( COND_CS()    ) ) /* BCS         */
OP( b1, 7, OP_LD    ( REG_A, IDY   ) ) /* LDA idy     */
OP( b2, 7, OP_LD    ( REG_A, ZPI   ) ) /* LDA zpi (C) */
OP( b3, 8, OP_TST   ( ABX          ) ) /* TST abx (H) */
OP( b4, 4, OP_LD    ( REG_Y, ZPX   ) ) /* LDY zpx     */
OP( b5, 4, OP_LD    ( REG_A, ZPX   ) ) /* LDA zpx     */
OP( b6, 4, OP_LD    ( REG_X, ZPY   ) ) /* LDX zpy     */
OP( b7, 7, OP_SMB   ( ZPG, BIT_3   ) ) /* SMB3    (C) */
OP( b8, 2, OP_CLV   (              ) ) /* CLV         */
OP( b9, 5, OP_LD    ( REG_A, ABY   ) ) /* LDA aby     */
OP( ba, 2, OP_TRANS ( REG_S, REG_X ) ) /* TSX         */
OP( bb, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( bc, 5, OP_LD    ( REG_Y, ABX   ) ) /* LDY abx     */
OP( bd, 5, OP_LD    ( REG_A, ABX   ) ) /* LDA abx     */
OP( be, 5, OP_LD    ( REG_X, ABY   ) ) /* LDX aby     */
OP( bf, 0, OP_BBS   ( BIT_3        ) ) /* BBS3    (C) */
OP( c0, 2, OP_CMP   ( REG_Y, IMM   ) ) /* CPY imm     */
OP( c1, 7, OP_CMP   ( REG_A, IDX   ) ) /* CMP idx     */
OP( c2, 2, OP_CLY   (              ) ) /* CLY         */
OP( c3, 0, OP_TDD   (              ) ) /* TDD     (H) */
OP( c4, 4, OP_CMP   ( REG_Y, ZPG   ) ) /* CPY zp      */
OP( c5, 4, OP_CMP   ( REG_A, ZPG   ) ) /* CMP zp      */
OP( c6, 6, OP_DEC_M ( ZPG          ) ) /* DEC zp      */
OP( c7, 7, OP_SMB   ( ZPG, BIT_4   ) ) /* SMB4    (C) */
OP( c8, 2, OP_INC_R ( REG_Y        ) ) /* INY         */
OP( c9, 2, OP_CMP   ( REG_A, IMM   ) ) /* CMP imm     */
OP( ca, 2, OP_DEC_R ( REG_X        ) ) /* DEX         */
OP( cb, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( cc, 5, OP_CMP   ( REG_Y, ABS   ) ) /* CPY abs     */
OP( cd, 5, OP_CMP   ( REG_A, ABS   ) ) /* CMP abs     */
OP( ce, 7, OP_DEC_M ( ABS          ) ) /* DEC abs     */
OP( cf, 0, OP_BBS   ( BIT_4        ) ) /* BBS4    (C) */
OP( d0, 0, OP_BCC   ( COND_NE()    ) ) /* BNE         */
OP( d1, 7, OP_CMP   ( REG_A, IDY   ) ) /* CMP idy     */
OP( d2, 7, OP_CMP   ( REG_A, ZPI   ) ) /* CMP zpi (C) */
OP( d3, 0, OP_TIN   (              ) ) /* TIN     (H) */
OP( d4, 2, OP_CSH   (              ) ) /* CSH     (H) */
OP( d5, 4, OP_CMP   ( REG_A, ZPX   ) ) /* CMP zpx     */
OP( d6, 6, OP_DEC_M ( ZPX          ) ) /* DEC zpx     */
OP( d7, 7, OP_SMB   ( ZPG, BIT_5   ) ) /* SMB5    (C) */
OP( d8, 2, OP_CLD   (              ) ) /* CLD         */
OP( d9, 5, OP_CMP   ( REG_A, ABY   ) ) /* CMP aby     */
OP( da, 3, OP_PUSH  ( REG_X        ) ) /* PHX     (C) */
OP( db, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( dc, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( dd, 5, OP_CMP   ( REG_A, ABX   ) ) /* CMP abx     */
OP( de, 7, OP_DEC_M ( ABX          ) ) /* DEC abx     */
OP( df, 0, OP_BBS   ( BIT_5        ) ) /* BBS5    (C) */
OP( e0, 2, OP_CMP   ( REG_X, IMM   ) ) /* CPX imm     */
OP( e1, 7, OP_SBC   ( IDX          ) ) /* SBC idx     */
OP( e2, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( e3, 0, OP_TIA   (              ) ) /* TIA     (H) */
OP( e4, 4, OP_CMP   ( REG_X, ZPG   ) ) /* CPX zp      */
OP( e5, 4, OP_SBC   ( ZPG          ) ) /* SBC zp      */
OP( e6, 4, OP_INC_M ( ZPG          ) ) /* INC zp      */
OP( e7, 7, OP_SMB   ( ZPG, BIT_6   ) ) /* SMB6    (C) */
OP( e8, 2, OP_INC_R ( REG_X        ) ) /* INX         */
OP( e9, 2, OP_SBC   ( IMM          ) ) /* SBC imm     */
OP( ea, 2, OP_NOP   (              ) ) /* NOP         */
OP( eb, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( ec, 5, OP_CMP   ( REG_X, ABS   ) ) /* CPX abs     */
OP( ed, 5, OP_SBC   ( ABS          ) ) /* SBC abs     */
OP( ee, 7, OP_INC_M ( ABS          ) ) /* INC abs     */
OP( ef, 0, OP_BBS   ( BIT_6        ) ) /* BBS6    (C) */
OP( f0, 0, OP_BCC   ( COND_EQ()    ) ) /* BEQ         */
OP( f1, 7, OP_SBC   ( IDY          ) ) /* SBC idy     */
OP( f2, 7, OP_SBC   ( ZPI          ) ) /* SBC zpi (C) */
OP( f3, 0, OP_TAI   (              ) ) /* TAI     (H) */
OP( f4, 2, OP_SET   (              ) ) /* SET     (H) */
OP( f5, 4, OP_SBC   ( ZPX          ) ) /* SBC zpx     */
OP( f6, 6, OP_INC_M ( ZPX          ) ) /* INC zpx     */
OP( f7, 7, OP_SMB   ( ZPG, BIT_7   ) ) /* SMB7    (C) */
OP( f8, 2, OP_SED   (              ) ) /* SED         */
OP( f9, 5, OP_SBC   ( ABY          ) ) /* SBC aby     */
OP( fa, 4, OP_PULL  ( REG_X        ) ) /* PLX     (C) */
OP( fb, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( fc, 2, OP_ILL   (              ) ) /* ILLEGAL     */
OP( fd, 5, OP_SBC   ( ABX          ) ) /* SBC abx     */
OP( fe, 7, OP_INC_M ( ABX          ) ) /* INC abx     */
OP( ff, 0, OP_BBS   ( BIT_7        ) ) /* BBS7    (C) */
OPCODES_END
