/*
 * Copyright 1997-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 */

#include "incls/_precompiled.incl"
#include "incls/_assembler_sparc.cpp.incl"

// Implementation of Address

Address::Address( addr_type t, int which ) {
  switch (t) {
   case extra_in_argument:
   case extra_out_argument:
     _base = t == extra_in_argument ? FP : SP;
     _hi   = 0;
// Warning:  In LP64 mode, _disp will occupy more than 10 bits.
//           This is inconsistent with the other constructors but op
//           codes such as ld or ldx, only access disp() to get their
//           simm13 argument.
     _disp = ((which - Argument::n_register_parameters + frame::memory_parameter_word_sp_offset) * BytesPerWord) + STACK_BIAS;
    break;
   default:
    ShouldNotReachHere();
    break;
  }
}

static const char* argumentNames[][2] = {
  {"A0","P0"}, {"A1","P1"}, {"A2","P2"}, {"A3","P3"}, {"A4","P4"},
  {"A5","P5"}, {"A6","P6"}, {"A7","P7"}, {"A8","P8"}, {"A9","P9"},
  {"A(n>9)","P(n>9)"}
};

const char* Argument::name() const {
  int nofArgs = sizeof argumentNames / sizeof argumentNames[0];
  int num = number();
  if (num >= nofArgs)  num = nofArgs - 1;
  return argumentNames[num][is_in() ? 1 : 0];
}

void Assembler::print_instruction(int inst) {
  const char* s;
  switch (inv_op(inst)) {
  default:         s = "????"; break;
  case call_op:    s = "call"; break;
  case branch_op:
    switch (inv_op2(inst)) {
      case bpr_op2:    s = "bpr";  break;
      case fb_op2:     s = "fb";   break;
      case fbp_op2:    s = "fbp";  break;
      case br_op2:     s = "br";   break;
      case bp_op2:     s = "bp";   break;
      case cb_op2:     s = "cb";   break;
      default:         s = "????"; break;
    }
  }
  ::tty->print("%s", s);
}


// Patch instruction inst at offset inst_pos to refer to dest_pos
// and return the resulting instruction.
// We should have pcs, not offsets, but since all is relative, it will work out
// OK.
int Assembler::patched_branch(int dest_pos, int inst, int inst_pos) {

  int m; // mask for displacement field
  int v; // new value for displacement field
  const int word_aligned_ones = -4;
  switch (inv_op(inst)) {
  default: ShouldNotReachHere();
  case call_op:    m = wdisp(word_aligned_ones, 0, 30);  v = wdisp(dest_pos, inst_pos, 30); break;
  case branch_op:
    switch (inv_op2(inst)) {
      case bpr_op2:    m = wdisp16(word_aligned_ones, 0);      v = wdisp16(dest_pos, inst_pos);     break;
      case fbp_op2:    m = wdisp(  word_aligned_ones, 0, 19);  v = wdisp(  dest_pos, inst_pos, 19); break;
      case bp_op2:     m = wdisp(  word_aligned_ones, 0, 19);  v = wdisp(  dest_pos, inst_pos, 19); break;
      case fb_op2:     m = wdisp(  word_aligned_ones, 0, 22);  v = wdisp(  dest_pos, inst_pos, 22); break;
      case br_op2:     m = wdisp(  word_aligned_ones, 0, 22);  v = wdisp(  dest_pos, inst_pos, 22); break;
      case cb_op2:     m = wdisp(  word_aligned_ones, 0, 22);  v = wdisp(  dest_pos, inst_pos, 22); break;
      default: ShouldNotReachHere();
    }
  }
  return  inst & ~m  |  v;
}

// Return the offset of the branch destionation of instruction inst
// at offset pos.
// Should have pcs, but since all is relative, it works out.
int Assembler::branch_destination(int inst, int pos) {
  int r;
  switch (inv_op(inst)) {
  default: ShouldNotReachHere();
  case call_op:        r = inv_wdisp(inst, pos, 30);  break;
  case branch_op:
    switch (inv_op2(inst)) {
      case bpr_op2:    r = inv_wdisp16(inst, pos);    break;
      case fbp_op2:    r = inv_wdisp(  inst, pos, 19);  break;
      case bp_op2:     r = inv_wdisp(  inst, pos, 19);  break;
      case fb_op2:     r = inv_wdisp(  inst, pos, 22);  break;
      case br_op2:     r = inv_wdisp(  inst, pos, 22);  break;
      case cb_op2:     r = inv_wdisp(  inst, pos, 22);  break;
      default: ShouldNotReachHere();
    }
  }
  return r;
}

int AbstractAssembler::code_fill_byte() {
  return 0x00;                  // illegal instruction 0x00000000
}

// Generate a bunch 'o stuff (including v9's
#ifndef PRODUCT
void Assembler::test_v9() {
  add(    G0, G1, G2 );
  add(    G3,  0, G4 );

  addcc(  G5, G6, G7 );
  addcc(  I0,  1, I1 );
  addc(   I2, I3, I4 );
  addc(   I5, -1, I6 );
  addccc( I7, L0, L1 );
  addccc( L2, (1 << 12) - 2, L3 );

  Label lbl1, lbl2, lbl3;

  bind(lbl1);

  bpr( rc_z,    true, pn, L4, pc(),  relocInfo::oop_type );
  delayed()->nop();
  bpr( rc_lez, false, pt, L5, lbl1);
  delayed()->nop();

  fb( f_never,     true, pc() + 4,  relocInfo::none);
  delayed()->nop();
  fb( f_notEqual, false, lbl2 );
  delayed()->nop();

  fbp( f_notZero,        true, fcc0, pn, pc() - 4,  relocInfo::none);
  delayed()->nop();
  fbp( f_lessOrGreater, false, fcc1, pt, lbl3 );
  delayed()->nop();

  br( equal,  true, pc() + 1024, relocInfo::none);
  delayed()->nop();
  br( lessEqual, false, lbl1 );
  delayed()->nop();
  br( never, false, lbl1 );
  delayed()->nop();

  bp( less,               true, icc, pn, pc(), relocInfo::none);
  delayed()->nop();
  bp( lessEqualUnsigned, false, xcc, pt, lbl2 );
  delayed()->nop();

  call( pc(), relocInfo::none);
  delayed()->nop();
  call( lbl3 );
  delayed()->nop();


  casa(  L6, L7, O0 );
  casxa( O1, O2, O3, 0 );

  udiv(   O4, O5, O7 );
  udiv(   G0, (1 << 12) - 1, G1 );
  sdiv(   G1, G2, G3 );
  sdiv(   G4, -((1 << 12) - 1), G5 );
  udivcc( G6, G7, I0 );
  udivcc( I1, -((1 << 12) - 2), I2 );
  sdivcc( I3, I4, I5 );
  sdivcc( I6, -((1 << 12) - 0), I7 );

  done();
  retry();

  fadd( FloatRegisterImpl::S, F0,  F1, F2 );
  fsub( FloatRegisterImpl::D, F34, F0, F62 );

  fcmp(  FloatRegisterImpl::Q, fcc0, F0, F60);
  fcmpe( FloatRegisterImpl::S, fcc1, F31, F30);

  ftox( FloatRegisterImpl::D, F2, F4 );
  ftoi( FloatRegisterImpl::Q, F4, F8 );

  ftof( FloatRegisterImpl::S, FloatRegisterImpl::Q, F3, F12 );

  fxtof( FloatRegisterImpl::S, F4, F5 );
  fitof( FloatRegisterImpl::D, F6, F8 );

  fmov( FloatRegisterImpl::Q, F16, F20 );
  fneg( FloatRegisterImpl::S, F6, F7 );
  fabs( FloatRegisterImpl::D, F10, F12 );

  fmul( FloatRegisterImpl::Q,  F24, F28, F32 );
  fmul( FloatRegisterImpl::S,  FloatRegisterImpl::D,  F8, F9, F14 );
  fdiv( FloatRegisterImpl::S,  F10, F11, F12 );

  fsqrt( FloatRegisterImpl::S, F13, F14 );

  flush( L0, L1 );
  flush( L2, -1 );

  flushw();

  illtrap( (1 << 22) - 2);

  impdep1( 17, (1 << 19) - 1 );
  impdep2( 3,  0 );

  jmpl( L3, L4, L5 );
  delayed()->nop();
  jmpl( L6, -1, L7, Relocation::spec_simple(relocInfo::none));
  delayed()->nop();


  ldf(    FloatRegisterImpl::S, O0, O1, F15 );
  ldf(    FloatRegisterImpl::D, O2, -1, F14 );


  ldfsr(  O3, O4 );
  ldfsr(  O5, -1 );
  ldxfsr( O6, O7 );
  ldxfsr( I0, -1 );

  ldfa(  FloatRegisterImpl::D, I1, I2, 1, F16 );
  ldfa(  FloatRegisterImpl::Q, I3, -1,    F36 );

  ldsb(  I4, I5, I6 );
  ldsb(  I7, -1, G0 );
  ldsh(  G1, G3, G4 );
  ldsh(  G5, -1, G6 );
  ldsw(  G7, L0, L1 );
  ldsw(  L2, -1, L3 );
  ldub(  L4, L5, L6 );
  ldub(  L7, -1, O0 );
  lduh(  O1, O2, O3 );
  lduh(  O4, -1, O5 );
  lduw(  O6, O7, G0 );
  lduw(  G1, -1, G2 );
  ldx(   G3, G4, G5 );
  ldx(   G6, -1, G7 );
  ldd(   I0, I1, I2 );
  ldd(   I3, -1, I4 );

  ldsba(  I5, I6, 2, I7 );
  ldsba(  L0, -1, L1 );
  ldsha(  L2, L3, 3, L4 );
  ldsha(  L5, -1, L6 );
  ldswa(  L7, O0, (1 << 8) - 1, O1 );
  ldswa(  O2, -1, O3 );
  lduba(  O4, O5, 0, O6 );
  lduba(  O7, -1, I0 );
  lduha(  I1, I2, 1, I3 );
  lduha(  I4, -1, I5 );
  lduwa(  I6, I7, 2, L0 );
  lduwa(  L1, -1, L2 );
  ldxa(   L3, L4, 3, L5 );
  ldxa(   L6, -1, L7 );
  ldda(   G0, G1, 4, G2 );
  ldda(   G3, -1, G4 );

  ldstub(  G5, G6, G7 );
  ldstub(  O0, -1, O1 );

  ldstuba( O2, O3, 5, O4 );
  ldstuba( O5, -1, O6 );

  and3(    I0, L0, O0 );
  and3(    G7, -1, O7 );
  andcc(   L2, I2, G2 );
  andcc(   L4, -1, G4 );
  andn(    I5, I6, I7 );
  andn(    I6, -1, I7 );
  andncc(  I5, I6, I7 );
  andncc(  I7, -1, I6 );
  or3(     I5, I6, I7 );
  or3(     I7, -1, I6 );
  orcc(    I5, I6, I7 );
  orcc(    I7, -1, I6 );
  orn(     I5, I6, I7 );
  orn(     I7, -1, I6 );
  orncc(   I5, I6, I7 );
  orncc(   I7, -1, I6 );
  xor3(    I5, I6, I7 );
  xor3(    I7, -1, I6 );
  xorcc(   I5, I6, I7 );
  xorcc(   I7, -1, I6 );
  xnor(    I5, I6, I7 );
  xnor(    I7, -1, I6 );
  xnorcc(  I5, I6, I7 );
  xnorcc(  I7, -1, I6 );

  membar( Membar_mask_bits(StoreStore | LoadStore | StoreLoad | LoadLoad | Sync | MemIssue | Lookaside ) );
  membar( StoreStore );
  membar( LoadStore );
  membar( StoreLoad );
  membar( LoadLoad );
  membar( Sync );
  membar( MemIssue );
  membar( Lookaside );

  fmov( FloatRegisterImpl::S, f_ordered,  true, fcc2, F16, F17 );
  fmov( FloatRegisterImpl::D, rc_lz, L5, F18, F20 );

  movcc( overflowClear,  false, icc, I6, L4 );
  movcc( f_unorderedOrEqual, true, fcc2, (1 << 10) - 1, O0 );

  movr( rc_nz, I5, I6, I7 );
  movr( rc_gz, L1, -1,  L2 );

  mulx(  I5, I6, I7 );
  mulx(  I7, -1, I6 );
  sdivx( I5, I6, I7 );
  sdivx( I7, -1, I6 );
  udivx( I5, I6, I7 );
  udivx( I7, -1, I6 );

  umul(   I5, I6, I7 );
  umul(   I7, -1, I6 );
  smul(   I5, I6, I7 );
  smul(   I7, -1, I6 );
  umulcc( I5, I6, I7 );
  umulcc( I7, -1, I6 );
  smulcc( I5, I6, I7 );
  smulcc( I7, -1, I6 );

  mulscc(   I5, I6, I7 );
  mulscc(   I7, -1, I6 );

  nop();


  popc( G0,  G1);
  popc( -1, G2);

  prefetch(   L1, L2,    severalReads );
  prefetch(   L3, -1,    oneRead );
  prefetcha(  O3, O2, 6, severalWritesAndPossiblyReads );
  prefetcha(  G2, -1,    oneWrite );

  rett( I7, I7);
  delayed()->nop();
  rett( G0, -1, relocInfo::none);
  delayed()->nop();

  save(    I5, I6, I7 );
  save(    I7, -1, I6 );
  restore( I5, I6, I7 );
  restore( I7, -1, I6 );

  saved();
  restored();

  sethi( 0xaaaaaaaa, I3, Relocation::spec_simple(relocInfo::none));

  sll(  I5, I6, I7 );
  sll(  I7, 31, I6 );
  srl(  I5, I6, I7 );
  srl(  I7,  0, I6 );
  sra(  I5, I6, I7 );
  sra(  I7, 30, I6 );
  sllx( I5, I6, I7 );
  sllx( I7, 63, I6 );
  srlx( I5, I6, I7 );
  srlx( I7,  0, I6 );
  srax( I5, I6, I7 );
  srax( I7, 62, I6 );

  sir( -1 );

  stbar();

  stf(    FloatRegisterImpl::Q, F40, G0, I7 );
  stf(    FloatRegisterImpl::S, F18, I3, -1 );

  stfsr(  L1, L2 );
  stfsr(  I7, -1 );
  stxfsr( I6, I5 );
  stxfsr( L4, -1 );

  stfa(  FloatRegisterImpl::D, F22, I6, I7, 7 );
  stfa(  FloatRegisterImpl::Q, F44, G0, -1 );

  stb(  L5, O2, I7 );
  stb(  I7, I6, -1 );
  sth(  L5, O2, I7 );
  sth(  I7, I6, -1 );
  stw(  L5, O2, I7 );
  stw(  I7, I6, -1 );
  stx(  L5, O2, I7 );
  stx(  I7, I6, -1 );
  std(  L5, O2, I7 );
  std(  I7, I6, -1 );

  stba(  L5, O2, I7, 8 );
  stba(  I7, I6, -1    );
  stha(  L5, O2, I7, 9 );
  stha(  I7, I6, -1    );
  stwa(  L5, O2, I7, 0 );
  stwa(  I7, I6, -1    );
  stxa(  L5, O2, I7, 11 );
  stxa(  I7, I6, -1     );
  stda(  L5, O2, I7, 12 );
  stda(  I7, I6, -1     );

  sub(    I5, I6, I7 );
  sub(    I7, -1, I6 );
  subcc(  I5, I6, I7 );
  subcc(  I7, -1, I6 );
  subc(   I5, I6, I7 );
  subc(   I7, -1, I6 );
  subccc( I5, I6, I7 );
  subccc( I7, -1, I6 );

  swap( I5, I6, I7 );
  swap( I7, -1, I6 );

  swapa(   G0, G1, 13, G2 );
  swapa(   I7, -1,     I6 );

  taddcc(    I5, I6, I7 );
  taddcc(    I7, -1, I6 );
  taddcctv(  I5, I6, I7 );
  taddcctv(  I7, -1, I6 );

  tsubcc(    I5, I6, I7 );
  tsubcc(    I7, -1, I6 );
  tsubcctv(  I5, I6, I7 );
  tsubcctv(  I7, -1, I6 );

  trap( overflowClear, xcc, G0, G1 );
  trap( lessEqual,     icc, I7, 17 );

  bind(lbl2);
  bind(lbl3);

  code()->decode();
}

// Generate a bunch 'o stuff unique to V8
void Assembler::test_v8_onlys() {
  Label lbl1;

  cb( cp_0or1or2, false, pc() - 4, relocInfo::none);
  delayed()->nop();
  cb( cp_never,    true, lbl1);
  delayed()->nop();

  cpop1(1, 2, 3, 4);
  cpop2(5, 6, 7, 8);

  ldc( I0, I1, 31);
  ldc( I2, -1,  0);

  lddc( I4, I4, 30);
  lddc( I6,  0, 1 );

  ldcsr( L0, L1, 0);
  ldcsr( L1, (1 << 12) - 1, 17 );

  stc( 31, L4, L5);
  stc( 30, L6, -(1 << 12) );

  stdc( 0, L7, G0);
  stdc( 1, G1, 0 );

  stcsr( 16, G2, G3);
  stcsr( 17, G4, 1 );

  stdcq( 4, G5, G6);
  stdcq( 5, G7, -1 );

  bind(lbl1);

  code()->decode();
}
#endif

// Implementation of MacroAssembler

void MacroAssembler::null_check(Register reg, int offset) {
  if (needs_explicit_null_check((intptr_t)offset)) {
    // provoke OS NULL exception if reg = NULL by
    // accessing M[reg] w/o changing any registers
    ld_ptr(reg, 0, G0);
  }
  else {
    // nothing to do, (later) access of M[reg + offset]
    // will provoke OS NULL exception if reg = NULL
  }
}

// Ring buffer jumps

#ifndef PRODUCT
void MacroAssembler::ret(  bool trace )   { if (trace) {
                                                    mov(I7, O7); // traceable register
                                                    JMP(O7, 2 * BytesPerInstWord);
                                                  } else {
                                                    jmpl( I7, 2 * BytesPerInstWord, G0 );
                                                  }
                                                }

void MacroAssembler::retl( bool trace )  { if (trace) JMP(O7, 2 * BytesPerInstWord);
                                                 else jmpl( O7, 2 * BytesPerInstWord, G0 ); }
#endif /* PRODUCT */


void MacroAssembler::jmp2(Register r1, Register r2, const char* file, int line ) {
  assert_not_delayed();
  // This can only be traceable if r1 & r2 are visible after a window save
  if (TraceJumps) {
#ifndef PRODUCT
    save_frame(0);
    verify_thread();
    ld(G2_thread, in_bytes(JavaThread::jmp_ring_index_offset()), O0);
    add(G2_thread, in_bytes(JavaThread::jmp_ring_offset()), O1);
    sll(O0, exact_log2(4*sizeof(intptr_t)), O2);
    add(O2, O1, O1);

    add(r1->after_save(), r2->after_save(), O2);
    set((intptr_t)file, O3);
    set(line, O4);
    Label L;
    // get nearby pc, store jmp target
    call(L, relocInfo::none);  // No relocation for call to pc+0x8
    delayed()->st(O2, O1, 0);
    bind(L);

    // store nearby pc
    st(O7, O1, sizeof(intptr_t));
    // store file
    st(O3, O1, 2*sizeof(intptr_t));
    // store line
    st(O4, O1, 3*sizeof(intptr_t));
    add(O0, 1, O0);
    and3(O0, JavaThread::jump_ring_buffer_size  - 1, O0);
    st(O0, G2_thread, in_bytes(JavaThread::jmp_ring_index_offset()));
    restore();
#endif /* PRODUCT */
  }
  jmpl(r1, r2, G0);
}
void MacroAssembler::jmp(Register r1, int offset, const char* file, int line ) {
  assert_not_delayed();
  // This can only be traceable if r1 is visible after a window save
  if (TraceJumps) {
#ifndef PRODUCT
    save_frame(0);
    verify_thread();
    ld(G2_thread, in_bytes(JavaThread::jmp_ring_index_offset()), O0);
    add(G2_thread, in_bytes(JavaThread::jmp_ring_offset()), O1);
    sll(O0, exact_log2(4*sizeof(intptr_t)), O2);
    add(O2, O1, O1);

    add(r1->after_save(), offset, O2);
    set((intptr_t)file, O3);
    set(line, O4);
    Label L;
    // get nearby pc, store jmp target
    call(L, relocInfo::none);  // No relocation for call to pc+0x8
    delayed()->st(O2, O1, 0);
    bind(L);

    // store nearby pc
    st(O7, O1, sizeof(intptr_t));
    // store file
    st(O3, O1, 2*sizeof(intptr_t));
    // store line
    st(O4, O1, 3*sizeof(intptr_t));
    add(O0, 1, O0);
    and3(O0, JavaThread::jump_ring_buffer_size  - 1, O0);
    st(O0, G2_thread, in_bytes(JavaThread::jmp_ring_index_offset()));
    restore();
#endif /* PRODUCT */
  }
  jmp(r1, offset);
}

// This code sequence is relocatable to any address, even on LP64.
void MacroAssembler::jumpl( Address& a, Register d, int offset, const char* file, int line ) {
  assert_not_delayed();
  // Force fixed length sethi because NativeJump and NativeFarCall don't handle
  // variable length instruction streams.
  sethi(a, /*ForceRelocatable=*/ true);
  if (TraceJumps) {
#ifndef PRODUCT
    // Must do the add here so relocation can find the remainder of the
    // value to be relocated.
    add(a.base(), a.disp() + offset, a.base(), a.rspec(offset));
    save_frame(0);
    verify_thread();
    ld(G2_thread, in_bytes(JavaThread::jmp_ring_index_offset()), O0);
    add(G2_thread, in_bytes(JavaThread::jmp_ring_offset()), O1);
    sll(O0, exact_log2(4*sizeof(intptr_t)), O2);
    add(O2, O1, O1);

    set((intptr_t)file, O3);
    set(line, O4);
    Label L;

    // get nearby pc, store jmp target
    call(L, relocInfo::none);  // No relocation for call to pc+0x8
    delayed()->st(a.base()->after_save(), O1, 0);
    bind(L);

    // store nearby pc
    st(O7, O1, sizeof(intptr_t));
    // store file
    st(O3, O1, 2*sizeof(intptr_t));
    // store line
    st(O4, O1, 3*sizeof(intptr_t));
    add(O0, 1, O0);
    and3(O0, JavaThread::jump_ring_buffer_size  - 1, O0);
    st(O0, G2_thread, in_bytes(JavaThread::jmp_ring_index_offset()));
    restore();
    jmpl(a.base(), G0, d);
#else
    jmpl(a, d, offset);
#endif /* PRODUCT */
  } else {
    jmpl(a, d, offset);
  }
}

void MacroAssembler::jump( Address& a, int offset, const char* file, int line ) {
  jumpl( a, G0, offset, file, line );
}


// Convert to C varargs format
void MacroAssembler::set_varargs( Argument inArg, Register d ) {
  // spill register-resident args to their memory slots
  // (SPARC calling convention requires callers to have already preallocated these)
  // Note that the inArg might in fact be an outgoing argument,
  // if a leaf routine or stub does some tricky argument shuffling.
  // This routine must work even though one of the saved arguments
  // is in the d register (e.g., set_varargs(Argument(0, false), O0)).
  for (Argument savePtr = inArg;
       savePtr.is_register();
       savePtr = savePtr.successor()) {
    st_ptr(savePtr.as_register(), savePtr.address_in_frame());
  }
  // return the address of the first memory slot
  add(inArg.address_in_frame(), d);
}

// Conditional breakpoint (for assertion checks in assembly code)
void MacroAssembler::breakpoint_trap(Condition c, CC cc) {
  trap(c, cc, G0, ST_RESERVED_FOR_USER_0);
}

// We want to use ST_BREAKPOINT here, but the debugger is confused by it.
void MacroAssembler::breakpoint_trap() {
  trap(ST_RESERVED_FOR_USER_0);
}

// flush windows (except current) using flushw instruction if avail.
void MacroAssembler::flush_windows() {
  if (VM_Version::v9_instructions_work())  flushw();
  else                                     flush_windows_trap();
}

// Write serialization page so VM thread can do a pseudo remote membar
// We use the current thread pointer to calculate a thread specific
// offset to write to within the page. This minimizes bus traffic
// due to cache line collision.
void MacroAssembler::serialize_memory(Register thread, Register tmp1, Register tmp2) {
  Address mem_serialize_page(tmp1, os::get_memory_serialize_page());
  srl(thread, os::get_serialize_page_shift_count(), tmp2);
  if (Assembler::is_simm13(os::vm_page_size())) {
    and3(tmp2, (os::vm_page_size() - sizeof(int)), tmp2);
  }
  else {
    set((os::vm_page_size() - sizeof(int)), tmp1);
    and3(tmp2, tmp1, tmp2);
  }
  load_address(mem_serialize_page);
  st(G0, tmp1, tmp2);
}



void MacroAssembler::enter() {
  Unimplemented();
}

void MacroAssembler::leave() {
  Unimplemented();
}

void MacroAssembler::mult(Register s1, Register s2, Register d) {
  if(VM_Version::v9_instructions_work()) {
    mulx (s1, s2, d);
  } else {
    smul (s1, s2, d);
  }
}

void MacroAssembler::mult(Register s1, int simm13a, Register d) {
  if(VM_Version::v9_instructions_work()) {
    mulx (s1, simm13a, d);
  } else {
    smul (s1, simm13a, d);
  }
}


#ifdef ASSERT
void MacroAssembler::read_ccr_v8_assert(Register ccr_save) {
  const Register s1 = G3_scratch;
  const Register s2 = G4_scratch;
  Label get_psr_test;
  // Get the condition codes the V8 way.
  read_ccr_trap(s1);
  mov(ccr_save, s2);
  // This is a test of V8 which has icc but not xcc
  // so mask off the xcc bits
  and3(s2, 0xf, s2);
  // Compare condition codes from the V8 and V9 ways.
  subcc(s2, s1, G0);
  br(Assembler::notEqual, true, Assembler::pt, get_psr_test);
  delayed()->breakpoint_trap();
  bind(get_psr_test);
}

void MacroAssembler::write_ccr_v8_assert(Register ccr_save) {
  const Register s1 = G3_scratch;
  const Register s2 = G4_scratch;
  Label set_psr_test;
  // Write out the saved condition codes the V8 way
  write_ccr_trap(ccr_save, s1, s2);
  // Read back the condition codes using the V9 instruction
  rdccr(s1);
  mov(ccr_save, s2);
  // This is a test of V8 which has icc but not xcc
  // so mask off the xcc bits
  and3(s2, 0xf, s2);
  and3(s1, 0xf, s1);
  // Compare the V8 way with the V9 way.
  subcc(s2, s1, G0);
  br(Assembler::notEqual, true, Assembler::pt, set_psr_test);
  delayed()->breakpoint_trap();
  bind(set_psr_test);
}
#else
#define read_ccr_v8_assert(x)
#define write_ccr_v8_assert(x)
#endif // ASSERT

void MacroAssembler::read_ccr(Register ccr_save) {
  if (VM_Version::v9_instructions_work()) {
    rdccr(ccr_save);
    // Test code sequence used on V8.  Do not move above rdccr.
    read_ccr_v8_assert(ccr_save);
  } else {
    read_ccr_trap(ccr_save);
  }
}

void MacroAssembler::write_ccr(Register ccr_save) {
  if (VM_Version::v9_instructions_work()) {
    // Test code sequence used on V8.  Do not move below wrccr.
    write_ccr_v8_assert(ccr_save);
    wrccr(ccr_save);
  } else {
    const Register temp_reg1 = G3_scratch;
    const Register temp_reg2 = G4_scratch;
    write_ccr_trap(ccr_save, temp_reg1, temp_reg2);
  }
}


// Calls to C land

#ifdef ASSERT
// a hook for debugging
static Thread* reinitialize_thread() {
  return ThreadLocalStorage::thread();
}
#else
#define reinitialize_thread ThreadLocalStorage::thread
#endif

#ifdef ASSERT
address last_get_thread = NULL;
#endif

// call this when G2_thread is not known to be valid
void MacroAssembler::get_thread() {
  save_frame(0);                // to avoid clobbering O0
  mov(G1, L0);                  // avoid clobbering G1
  mov(G5_method, L1);           // avoid clobbering G5
  mov(G3, L2);                  // avoid clobbering G3 also
  mov(G4, L5);                  // avoid clobbering G4
#ifdef ASSERT
  Address last_get_thread_addr(L3, (address)&last_get_thread);
  sethi(last_get_thread_addr);
  inc(L4, get_pc(L4) + 2 * BytesPerInstWord); // skip getpc() code + inc + st_ptr to point L4 at call
  st_ptr(L4, last_get_thread_addr);
#endif
  call(CAST_FROM_FN_PTR(address, reinitialize_thread), relocInfo::runtime_call_type);
  delayed()->nop();
  mov(L0, G1);
  mov(L1, G5_method);
  mov(L2, G3);
  mov(L5, G4);
  restore(O0, 0, G2_thread);
}

static Thread* verify_thread_subroutine(Thread* gthread_value) {
  Thread* correct_value = ThreadLocalStorage::thread();
  guarantee(gthread_value == correct_value, "G2_thread value must be the thread");
  return correct_value;
}

void MacroAssembler::verify_thread() {
  if (VerifyThread) {
    // NOTE: this chops off the heads of the 64-bit O registers.
#ifdef CC_INTERP
    save_frame(0);
#else
    // make sure G2_thread contains the right value
    save_frame_and_mov(0, Lmethod, Lmethod);   // to avoid clobbering O0 (and propagate Lmethod for -Xprof)
    mov(G1, L1);                // avoid clobbering G1
    // G2 saved below
    mov(G3, L3);                // avoid clobbering G3
    mov(G4, L4);                // avoid clobbering G4
    mov(G5_method, L5);         // avoid clobbering G5_method
#endif /* CC_INTERP */
#if defined(COMPILER2) && !defined(_LP64)
    // Save & restore possible 64-bit Long arguments in G-regs
    srlx(G1,32,L0);
    srlx(G4,32,L6);
#endif
    call(CAST_FROM_FN_PTR(address,verify_thread_subroutine), relocInfo::runtime_call_type);
    delayed()->mov(G2_thread, O0);

    mov(L1, G1);                // Restore G1
    // G2 restored below
    mov(L3, G3);                // restore G3
    mov(L4, G4);                // restore G4
    mov(L5, G5_method);         // restore G5_method
#if defined(COMPILER2) && !defined(_LP64)
    // Save & restore possible 64-bit Long arguments in G-regs
    sllx(L0,32,G2);             // Move old high G1 bits high in G2
    sllx(G1, 0,G1);             // Clear current high G1 bits
    or3 (G1,G2,G1);             // Recover 64-bit G1
    sllx(L6,32,G2);             // Move old high G4 bits high in G2
    sllx(G4, 0,G4);             // Clear current high G4 bits
    or3 (G4,G2,G4);             // Recover 64-bit G4
#endif
    restore(O0, 0, G2_thread);
  }
}


void MacroAssembler::save_thread(const Register thread_cache) {
  verify_thread();
  if (thread_cache->is_valid()) {
    assert(thread_cache->is_local() || thread_cache->is_in(), "bad volatile");
    mov(G2_thread, thread_cache);
  }
  if (VerifyThread) {
    // smash G2_thread, as if the VM were about to anyway
    set(0x67676767, G2_thread);
  }
}


void MacroAssembler::restore_thread(const Register thread_cache) {
  if (thread_cache->is_valid()) {
    assert(thread_cache->is_local() || thread_cache->is_in(), "bad volatile");
    mov(thread_cache, G2_thread);
    verify_thread();
  } else {
    // do it the slow way
    get_thread();
  }
}


// %%% maybe get rid of [re]set_last_Java_frame
void MacroAssembler::set_last_Java_frame(Register last_java_sp, Register last_Java_pc) {
  assert_not_delayed();
  Address flags(G2_thread,
                0,
                in_bytes(JavaThread::frame_anchor_offset()) +
                         in_bytes(JavaFrameAnchor::flags_offset()));
  Address pc_addr(G2_thread,
                  0,
                  in_bytes(JavaThread::last_Java_pc_offset()));

  // Always set last_Java_pc and flags first because once last_Java_sp is visible
  // has_last_Java_frame is true and users will look at the rest of the fields.
  // (Note: flags should always be zero before we get here so doesn't need to be set.)

#ifdef ASSERT
  // Verify that flags was zeroed on return to Java
  Label PcOk;
  save_frame(0);                // to avoid clobbering O0
  ld_ptr(pc_addr, L0);
  tst(L0);
#ifdef _LP64
  brx(Assembler::zero, false, Assembler::pt, PcOk);
#else
  br(Assembler::zero, false, Assembler::pt, PcOk);
#endif // _LP64
  delayed() -> nop();
  stop("last_Java_pc not zeroed before leaving Java");
  bind(PcOk);

  // Verify that flags was zeroed on return to Java
  Label FlagsOk;
  ld(flags, L0);
  tst(L0);
  br(Assembler::zero, false, Assembler::pt, FlagsOk);
  delayed() -> restore();
  stop("flags not zeroed before leaving Java");
  bind(FlagsOk);
#endif /* ASSERT */
  //
  // When returning from calling out from Java mode the frame anchor's last_Java_pc
  // will always be set to NULL. It is set here so that if we are doing a call to
  // native (not VM) that we capture the known pc and don't have to rely on the
  // native call having a standard frame linkage where we can find the pc.

  if (last_Java_pc->is_valid()) {
    st_ptr(last_Java_pc, pc_addr);
  }

#ifdef _LP64
#ifdef ASSERT
  // Make sure that we have an odd stack
  Label StackOk;
  andcc(last_java_sp, 0x01, G0);
  br(Assembler::notZero, false, Assembler::pt, StackOk);
  delayed() -> nop();
  stop("Stack Not Biased in set_last_Java_frame");
  bind(StackOk);
#endif // ASSERT
  assert( last_java_sp != G4_scratch, "bad register usage in set_last_Java_frame");
  add( last_java_sp, STACK_BIAS, G4_scratch );
  st_ptr(G4_scratch,    Address(G2_thread, 0, in_bytes(JavaThread::last_Java_sp_offset())));
#else
  st_ptr(last_java_sp,    Address(G2_thread, 0, in_bytes(JavaThread::last_Java_sp_offset())));
#endif // _LP64
}

void MacroAssembler::reset_last_Java_frame(void) {
  assert_not_delayed();

  Address sp_addr(G2_thread, 0, in_bytes(JavaThread::last_Java_sp_offset()));
  Address pc_addr(G2_thread,
                  0,
                  in_bytes(JavaThread::frame_anchor_offset()) + in_bytes(JavaFrameAnchor::last_Java_pc_offset()));
  Address flags(G2_thread,
                0,
                in_bytes(JavaThread::frame_anchor_offset()) + in_bytes(JavaFrameAnchor::flags_offset()));

#ifdef ASSERT
  // check that it WAS previously set
#ifdef CC_INTERP
    save_frame(0);
#else
    save_frame_and_mov(0, Lmethod, Lmethod);     // Propagate Lmethod to helper frame for -Xprof
#endif /* CC_INTERP */
    ld_ptr(sp_addr, L0);
    tst(L0);
    breakpoint_trap(Assembler::zero, Assembler::ptr_cc);
    restore();
#endif // ASSERT

  st_ptr(G0, sp_addr);
  // Always return last_Java_pc to zero
  st_ptr(G0, pc_addr);
  // Always null flags after return to Java
  st(G0, flags);
}


void MacroAssembler::call_VM_base(
  Register        oop_result,
  Register        thread_cache,
  Register        last_java_sp,
  address         entry_point,
  int             number_of_arguments,
  bool            check_exceptions)
{
  assert_not_delayed();

  // determine last_java_sp register
  if (!last_java_sp->is_valid()) {
    last_java_sp = SP;
  }
  // debugging support
  assert(number_of_arguments >= 0   , "cannot have negative number of arguments");

  // 64-bit last_java_sp is biased!
  set_last_Java_frame(last_java_sp, noreg);
  if (VerifyThread)  mov(G2_thread, O0); // about to be smashed; pass early
  save_thread(thread_cache);
  // do the call
  call(entry_point, relocInfo::runtime_call_type);
  if (!VerifyThread)
    delayed()->mov(G2_thread, O0);  // pass thread as first argument
  else
    delayed()->nop();             // (thread already passed)
  restore_thread(thread_cache);
  reset_last_Java_frame();

  // check for pending exceptions. use Gtemp as scratch register.
  if (check_exceptions) {
    check_and_forward_exception(Gtemp);
  }

  // get oop result if there is one and reset the value in the thread
  if (oop_result->is_valid()) {
    get_vm_result(oop_result);
  }
}

void MacroAssembler::check_and_forward_exception(Register scratch_reg)
{
  Label L;

  check_and_handle_popframe(scratch_reg);
  check_and_handle_earlyret(scratch_reg);

  Address exception_addr(G2_thread, 0, in_bytes(Thread::pending_exception_offset()));
  ld_ptr(exception_addr, scratch_reg);
  br_null(scratch_reg,false,pt,L);
  delayed()->nop();
  // we use O7 linkage so that forward_exception_entry has the issuing PC
  call(StubRoutines::forward_exception_entry(), relocInfo::runtime_call_type);
  delayed()->nop();
  bind(L);
}


void MacroAssembler::check_and_handle_popframe(Register scratch_reg) {
}


void MacroAssembler::check_and_handle_earlyret(Register scratch_reg) {
}


void MacroAssembler::call_VM(Register oop_result, address entry_point, int number_of_arguments, bool check_exceptions) {
  call_VM_base(oop_result, noreg, noreg, entry_point, number_of_arguments, check_exceptions);
}


void MacroAssembler::call_VM(Register oop_result, address entry_point, Register arg_1, bool check_exceptions) {
  // O0 is reserved for the thread
  mov(arg_1, O1);
  call_VM(oop_result, entry_point, 1, check_exceptions);
}


void MacroAssembler::call_VM(Register oop_result, address entry_point, Register arg_1, Register arg_2, bool check_exceptions) {
  // O0 is reserved for the thread
  mov(arg_1, O1);
  mov(arg_2, O2); assert(arg_2 != O1, "smashed argument");
  call_VM(oop_result, entry_point, 2, check_exceptions);
}


void MacroAssembler::call_VM(Register oop_result, address entry_point, Register arg_1, Register arg_2, Register arg_3, bool check_exceptions) {
  // O0 is reserved for the thread
  mov(arg_1, O1);
  mov(arg_2, O2); assert(arg_2 != O1,                "smashed argument");
  mov(arg_3, O3); assert(arg_3 != O1 && arg_3 != O2, "smashed argument");
  call_VM(oop_result, entry_point, 3, check_exceptions);
}



// Note: The following call_VM overloadings are useful when a "save"
// has already been performed by a stub, and the last Java frame is
// the previous one.  In that case, last_java_sp must be passed as FP
// instead of SP.


void MacroAssembler::call_VM(Register oop_result, Register last_java_sp, address entry_point, int number_of_arguments, bool check_exceptions) {
  call_VM_base(oop_result, noreg, last_java_sp, entry_point, number_of_arguments, check_exceptions);
}


void MacroAssembler::call_VM(Register oop_result, Register last_java_sp, address entry_point, Register arg_1, bool check_exceptions) {
  // O0 is reserved for the thread
  mov(arg_1, O1);
  call_VM(oop_result, last_java_sp, entry_point, 1, check_exceptions);
}


void MacroAssembler::call_VM(Register oop_result, Register last_java_sp, address entry_point, Register arg_1, Register arg_2, bool check_exceptions) {
  // O0 is reserved for the thread
  mov(arg_1, O1);
  mov(arg_2, O2); assert(arg_2 != O1, "smashed argument");
  call_VM(oop_result, last_java_sp, entry_point, 2, check_exceptions);
}


void MacroAssembler::call_VM(Register oop_result, Register last_java_sp, address entry_point, Register arg_1, Register arg_2, Register arg_3, bool check_exceptions) {
  // O0 is reserved for the thread
  mov(arg_1, O1);
  mov(arg_2, O2); assert(arg_2 != O1,                "smashed argument");
  mov(arg_3, O3); assert(arg_3 != O1 && arg_3 != O2, "smashed argument");
  call_VM(oop_result, last_java_sp, entry_point, 3, check_exceptions);
}



void MacroAssembler::call_VM_leaf_base(Register thread_cache, address entry_point, int number_of_arguments) {
  assert_not_delayed();
  save_thread(thread_cache);
  // do the call
  call(entry_point, relocInfo::runtime_call_type);
  delayed()->nop();
  restore_thread(thread_cache);
}


void MacroAssembler::call_VM_leaf(Register thread_cache, address entry_point, int number_of_arguments) {
  call_VM_leaf_base(thread_cache, entry_point, number_of_arguments);
}


void MacroAssembler::call_VM_leaf(Register thread_cache, address entry_point, Register arg_1) {
  mov(arg_1, O0);
  call_VM_leaf(thread_cache, entry_point, 1);
}


void MacroAssembler::call_VM_leaf(Register thread_cache, address entry_point, Register arg_1, Register arg_2) {
  mov(arg_1, O0);
  mov(arg_2, O1); assert(arg_2 != O0, "smashed argument");
  call_VM_leaf(thread_cache, entry_point, 2);
}


void MacroAssembler::call_VM_leaf(Register thread_cache, address entry_point, Register arg_1, Register arg_2, Register arg_3) {
  mov(arg_1, O0);
  mov(arg_2, O1); assert(arg_2 != O0,                "smashed argument");
  mov(arg_3, O2); assert(arg_3 != O0 && arg_3 != O1, "smashed argument");
  call_VM_leaf(thread_cache, entry_point, 3);
}


void MacroAssembler::get_vm_result(Register oop_result) {
  verify_thread();
  Address vm_result_addr(G2_thread, 0, in_bytes(JavaThread::vm_result_offset()));
  ld_ptr(    vm_result_addr, oop_result);
  st_ptr(G0, vm_result_addr);
  verify_oop(oop_result);
}


void MacroAssembler::get_vm_result_2(Register oop_result) {
  verify_thread();
  Address vm_result_addr_2(G2_thread, 0, in_bytes(JavaThread::vm_result_2_offset()));
  ld_ptr(vm_result_addr_2, oop_result);
  st_ptr(G0, vm_result_addr_2);
  verify_oop(oop_result);
}


// We require that C code which does not return a value in vm_result will
// leave it undisturbed.
void MacroAssembler::set_vm_result(Register oop_result) {
  verify_thread();
  Address vm_result_addr(G2_thread, 0, in_bytes(JavaThread::vm_result_offset()));
  verify_oop(oop_result);

# ifdef ASSERT
    // Check that we are not overwriting any other oop.
#ifdef CC_INTERP
    save_frame(0);
#else
    save_frame_and_mov(0, Lmethod, Lmethod);     // Propagate Lmethod for -Xprof
#endif /* CC_INTERP */
    ld_ptr(vm_result_addr, L0);
    tst(L0);
    restore();
    breakpoint_trap(notZero, Assembler::ptr_cc);
    // }
# endif

  st_ptr(oop_result, vm_result_addr);
}


void MacroAssembler::store_check(Register tmp, Register obj) {
  // Use two shifts to clear out those low order two bits! (Cannot opt. into 1.)

  /* $$$ This stuff needs to go into one of the BarrierSet generator
     functions.  (The particular barrier sets will have to be friends of
     MacroAssembler, I guess.) */
  BarrierSet* bs = Universe::heap()->barrier_set();
  assert(bs->kind() == BarrierSet::CardTableModRef, "Wrong barrier set kind");
  CardTableModRefBS* ct = (CardTableModRefBS*)bs;
  assert(sizeof(*ct->byte_map_base) == sizeof(jbyte), "adjust this code");
#ifdef _LP64
  srlx(obj, CardTableModRefBS::card_shift, obj);
#else
  srl(obj, CardTableModRefBS::card_shift, obj);
#endif
  assert( tmp != obj, "need separate temp reg");
  Address rs(tmp, (address)ct->byte_map_base);
  load_address(rs);
  stb(G0, rs.base(), obj);
}

void MacroAssembler::store_check(Register tmp, Register obj, Register offset) {
  store_check(tmp, obj);
}

// %%% Note:  The following six instructions have been moved,
//            unchanged, from assembler_sparc.inline.hpp.
//            They will be refactored at a later date.

void MacroAssembler::sethi(intptr_t imm22a,
                            Register d,
                            bool ForceRelocatable,
                            RelocationHolder const& rspec) {
  Address adr( d, (address)imm22a, rspec );
  MacroAssembler::sethi( adr, ForceRelocatable );
}


void MacroAssembler::sethi(Address& a, bool ForceRelocatable) {
  address save_pc;
  int shiftcnt;
  // if addr of local, do not need to load it
  assert(a.base() != FP  &&  a.base() != SP, "just use ld or st for locals");
#ifdef _LP64
# ifdef CHECK_DELAY
  assert_not_delayed( (char *)"cannot put two instructions in delay slot" );
# endif
  v9_dep();
//  ForceRelocatable = 1;
  save_pc = pc();
  if (a.hi32() == 0 && a.low32() >= 0) {
    Assembler::sethi(a.low32(), a.base(), a.rspec());
  }
  else if (a.hi32() == -1) {
    Assembler::sethi(~a.low32(), a.base(), a.rspec());
    xor3(a.base(), ~low10(~0), a.base());
  }
  else {
    Assembler::sethi(a.hi32(), a.base(), a.rspec() );   // 22
    if ( a.hi32() & 0x3ff )                     // Any bits?
      or3( a.base(), a.hi32() & 0x3ff ,a.base() ); // High 32 bits are now in low 32
    if ( a.low32() & 0xFFFFFC00 ) {             // done?
      if( (a.low32() >> 20) & 0xfff ) {         // Any bits set?
        sllx(a.base(), 12, a.base());           // Make room for next 12 bits
        or3( a.base(), (a.low32() >> 20) & 0xfff,a.base() ); // Or in next 12
        shiftcnt = 0;                           // We already shifted
      }
      else
        shiftcnt = 12;
      if( (a.low32() >> 10) & 0x3ff ) {
        sllx(a.base(), shiftcnt+10, a.base());// Make room for last 10 bits
        or3( a.base(), (a.low32() >> 10) & 0x3ff,a.base() ); // Or in next 10
        shiftcnt = 0;
      }
      else
        shiftcnt = 10;
      sllx(a.base(), shiftcnt+10 , a.base());           // Shift leaving disp field 0'd
    }
    else
      sllx( a.base(), 32, a.base() );
  }
  // Pad out the instruction sequence so it can be
  // patched later.
  if ( ForceRelocatable || (a.rtype() != relocInfo::none &&
                            a.rtype() != relocInfo::runtime_call_type) ) {
    while ( pc() < (save_pc + (7 * BytesPerInstWord )) )
      nop();
  }
#else
  Assembler::sethi(a.hi(), a.base(), a.rspec());
#endif

}

int MacroAssembler::size_of_sethi(address a, bool worst_case) {
#ifdef _LP64
  if (worst_case) return 7;
  intptr_t iaddr = (intptr_t)a;
  int hi32 = (int)(iaddr >> 32);
  int lo32 = (int)(iaddr);
  int inst_count;
  if (hi32 == 0 && lo32 >= 0)
    inst_count = 1;
  else if (hi32 == -1)
    inst_count = 2;
  else {
    inst_count = 2;
    if ( hi32 & 0x3ff )
      inst_count++;
    if ( lo32 & 0xFFFFFC00 ) {
      if( (lo32 >> 20) & 0xfff ) inst_count += 2;
      if( (lo32 >> 10) & 0x3ff ) inst_count += 2;
    }
  }
  return BytesPerInstWord * inst_count;
#else
  return BytesPerInstWord;
#endif
}

int MacroAssembler::worst_case_size_of_set() {
  return size_of_sethi(NULL, true) + 1;
}

void MacroAssembler::set(intptr_t value, Register d,
                         RelocationHolder const& rspec) {
  Address val( d, (address)value, rspec);

  if ( rspec.type() == relocInfo::none ) {
    // can optimize
    if (-4096 <= value  &&  value <= 4095) {
      or3(G0, value, d); // setsw (this leaves upper 32 bits sign-extended)
      return;
    }
    if (inv_hi22(hi22(value)) == value) {
      sethi(val);
      return;
    }
  }
  assert_not_delayed( (char *)"cannot put two instructions in delay slot" );
  sethi( val );
  if (rspec.type() != relocInfo::none || (value & 0x3ff) != 0) {
    add( d, value &  0x3ff, d, rspec);
  }
}

void MacroAssembler::setsw(int value, Register d,
                           RelocationHolder const& rspec) {
  Address val( d, (address)value, rspec);
  if ( rspec.type() == relocInfo::none ) {
    // can optimize
    if (-4096 <= value  &&  value <= 4095) {
      or3(G0, value, d);
      return;
    }
    if (inv_hi22(hi22(value)) == value) {
      sethi( val );
#ifndef _LP64
      if ( value < 0 ) {
        assert_not_delayed();
        sra (d, G0, d);
      }
#endif
      return;
    }
  }
  assert_not_delayed();
  sethi( val );
  add( d, value &  0x3ff, d, rspec);

  // (A negative value could be loaded in 2 insns with sethi/xor,
  // but it would take a more complex relocation.)
#ifndef _LP64
  if ( value < 0)
    sra(d, G0, d);
#endif
}

// %%% End of moved six set instructions.


void MacroAssembler::set64(jlong value, Register d, Register tmp) {
  assert_not_delayed();
  v9_dep();

  int hi = (int)(value >> 32);
  int lo = (int)(value & ~0);
  // (Matcher::isSimpleConstant64 knows about the following optimizations.)
  if (Assembler::is_simm13(lo) && value == lo) {
    or3(G0, lo, d);
  } else if (hi == 0) {
    Assembler::sethi(lo, d);   // hardware version zero-extends to upper 32
    if (low10(lo) != 0)
      or3(d, low10(lo), d);
  }
  else if (hi == -1) {
    Assembler::sethi(~lo, d);  // hardware version zero-extends to upper 32
    xor3(d, low10(lo) ^ ~low10(~0), d);
  }
  else if (lo == 0) {
    if (Assembler::is_simm13(hi)) {
      or3(G0, hi, d);
    } else {
      Assembler::sethi(hi, d);   // hardware version zero-extends to upper 32
      if (low10(hi) != 0)
        or3(d, low10(hi), d);
    }
    sllx(d, 32, d);
  }
  else {
    Assembler::sethi(hi, tmp);
    Assembler::sethi(lo,   d); // macro assembler version sign-extends
    if (low10(hi) != 0)
      or3 (tmp, low10(hi), tmp);
    if (low10(lo) != 0)
      or3 (  d, low10(lo),   d);
    sllx(tmp, 32, tmp);
    or3 (d, tmp, d);
  }
}

// compute size in bytes of sparc frame, given
// number of extraWords
int MacroAssembler::total_frame_size_in_bytes(int extraWords) {

  int nWords = frame::memory_parameter_word_sp_offset;

  nWords += extraWords;

  if (nWords & 1) ++nWords; // round up to double-word

  return nWords * BytesPerWord;
}


// save_frame: given number of "extra" words in frame,
// issue approp. save instruction (p 200, v8 manual)

void MacroAssembler::save_frame(int extraWords = 0) {
  int delta = -total_frame_size_in_bytes(extraWords);
  if (is_simm13(delta)) {
    save(SP, delta, SP);
  } else {
    set(delta, G3_scratch);
    save(SP, G3_scratch, SP);
  }
}


void MacroAssembler::save_frame_c1(int size_in_bytes) {
  if (is_simm13(-size_in_bytes)) {
    save(SP, -size_in_bytes, SP);
  } else {
    set(-size_in_bytes, G3_scratch);
    save(SP, G3_scratch, SP);
  }
}


void MacroAssembler::save_frame_and_mov(int extraWords,
                                        Register s1, Register d1,
                                        Register s2, Register d2) {
  assert_not_delayed();

  // The trick here is to use precisely the same memory word
  // that trap handlers also use to save the register.
  // This word cannot be used for any other purpose, but
  // it works fine to save the register's value, whether or not
  // an interrupt flushes register windows at any given moment!
  Address s1_addr;
  if (s1->is_valid() && (s1->is_in() || s1->is_local())) {
    s1_addr = s1->address_in_saved_window();
    st_ptr(s1, s1_addr);
  }

  Address s2_addr;
  if (s2->is_valid() && (s2->is_in() || s2->is_local())) {
    s2_addr = s2->address_in_saved_window();
    st_ptr(s2, s2_addr);
  }

  save_frame(extraWords);

  if (s1_addr.base() == SP) {
    ld_ptr(s1_addr.after_save(), d1);
  } else if (s1->is_valid()) {
    mov(s1->after_save(), d1);
  }

  if (s2_addr.base() == SP) {
    ld_ptr(s2_addr.after_save(), d2);
  } else if (s2->is_valid()) {
    mov(s2->after_save(), d2);
  }
}


Address MacroAssembler::allocate_oop_address(jobject obj, Register d) {
  assert(oop_recorder() != NULL, "this assembler needs an OopRecorder");
  int oop_index = oop_recorder()->allocate_index(obj);
  return Address(d, address(obj), oop_Relocation::spec(oop_index));
}


Address MacroAssembler::constant_oop_address(jobject obj, Register d) {
  assert(oop_recorder() != NULL, "this assembler needs an OopRecorder");
  int oop_index = oop_recorder()->find_index(obj);
  return Address(d, address(obj), oop_Relocation::spec(oop_index));
}

void  MacroAssembler::set_narrow_oop(jobject obj, Register d) {
  assert(oop_recorder() != NULL, "this assembler needs an OopRecorder");
  int oop_index = oop_recorder()->find_index(obj);
  RelocationHolder rspec = oop_Relocation::spec(oop_index);

  assert_not_delayed();
  // Relocation with special format (see relocInfo_sparc.hpp).
  relocate(rspec, 1);
  // Assembler::sethi(0x3fffff, d);
  emit_long( op(branch_op) | rd(d) | op2(sethi_op2) | hi22(0x3fffff) );
  // Don't add relocation for 'add'. Do patching during 'sethi' processing.
  add(d, 0x3ff, d);

}


void MacroAssembler::align(int modulus) {
  while (offset() % modulus != 0) nop();
}


void MacroAssembler::safepoint() {
  relocate(breakpoint_Relocation::spec(breakpoint_Relocation::safepoint));
}


void RegistersForDebugging::print(outputStream* s) {
  int j;
  for ( j = 0;  j < 8;  ++j )
    if ( j != 6 ) s->print_cr("i%d = 0x%.16lx", j, i[j]);
    else          s->print_cr( "fp = 0x%.16lx",    i[j]);
  s->cr();

  for ( j = 0;  j < 8;  ++j )
    s->print_cr("l%d = 0x%.16lx", j, l[j]);
  s->cr();

  for ( j = 0;  j < 8;  ++j )
    if ( j != 6 ) s->print_cr("o%d = 0x%.16lx", j, o[j]);
    else          s->print_cr( "sp = 0x%.16lx",    o[j]);
  s->cr();

  for ( j = 0;  j < 8;  ++j )
    s->print_cr("g%d = 0x%.16lx", j, g[j]);
  s->cr();

  // print out floats with compression
  for (j = 0; j < 32; ) {
    jfloat val = f[j];
    int last = j;
    for ( ;  last+1 < 32;  ++last ) {
      char b1[1024], b2[1024];
      sprintf(b1, "%f", val);
      sprintf(b2, "%f", f[last+1]);
      if (strcmp(b1, b2))
        break;
    }
    s->print("f%d", j);
    if ( j != last )  s->print(" - f%d", last);
    s->print(" = %f", val);
    s->fill_to(25);
    s->print_cr(" (0x%x)", val);
    j = last + 1;
  }
  s->cr();

  // and doubles (evens only)
  for (j = 0; j < 32; ) {
    jdouble val = d[j];
    int last = j;
    for ( ;  last+1 < 32;  ++last ) {
      char b1[1024], b2[1024];
      sprintf(b1, "%f", val);
      sprintf(b2, "%f", d[last+1]);
      if (strcmp(b1, b2))
        break;
    }
    s->print("d%d", 2 * j);
    if ( j != last )  s->print(" - d%d", last);
    s->print(" = %f", val);
    s->fill_to(30);
    s->print("(0x%x)", *(int*)&val);
    s->fill_to(42);
    s->print_cr("(0x%x)", *(1 + (int*)&val));
    j = last + 1;
  }
  s->cr();
}

void RegistersForDebugging::save_registers(MacroAssembler* a) {
  a->sub(FP, round_to(sizeof(RegistersForDebugging), sizeof(jdouble)) - STACK_BIAS, O0);
  a->flush_windows();
  int i;
  for (i = 0; i < 8; ++i) {
    a->ld_ptr(as_iRegister(i)->address_in_saved_window().after_save(), L1);  a->st_ptr( L1, O0, i_offset(i));
    a->ld_ptr(as_lRegister(i)->address_in_saved_window().after_save(), L1);  a->st_ptr( L1, O0, l_offset(i));
    a->st_ptr(as_oRegister(i)->after_save(), O0, o_offset(i));
    a->st_ptr(as_gRegister(i)->after_save(), O0, g_offset(i));
  }
  for (i = 0;  i < 32; ++i) {
    a->stf(FloatRegisterImpl::S, as_FloatRegister(i), O0, f_offset(i));
  }
  for (i = 0; i < (VM_Version::v9_instructions_work() ? 64 : 32); i += 2) {
    a->stf(FloatRegisterImpl::D, as_FloatRegister(i), O0, d_offset(i));
  }
}

void RegistersForDebugging::restore_registers(MacroAssembler* a, Register r) {
  for (int i = 1; i < 8;  ++i) {
    a->ld_ptr(r, g_offset(i), as_gRegister(i));
  }
  for (int j = 0; j < 32; ++j) {
    a->ldf(FloatRegisterImpl::S, O0, f_offset(j), as_FloatRegister(j));
  }
  for (int k = 0; k < (VM_Version::v9_instructions_work() ? 64 : 32); k += 2) {
    a->ldf(FloatRegisterImpl::D, O0, d_offset(k), as_FloatRegister(k));
  }
}


// pushes double TOS element of FPU stack on CPU stack; pops from FPU stack
void MacroAssembler::push_fTOS() {
  // %%%%%% need to implement this
}

// pops double TOS element from CPU stack and pushes on FPU stack
void MacroAssembler::pop_fTOS() {
  // %%%%%% need to implement this
}

void MacroAssembler::empty_FPU_stack() {
  // %%%%%% need to implement this
}

void MacroAssembler::_verify_oop(Register reg, const char* msg, const char * file, int line) {
  // plausibility check for oops
  if (!VerifyOops) return;

  if (reg == G0)  return;       // always NULL, which is always an oop

  char buffer[16];
  sprintf(buffer, "%d", line);
  int len = strlen(file) + strlen(msg) + 1 + 4 + strlen(buffer);
  char * real_msg = new char[len];
  sprintf(real_msg, "%s (%s:%d)", msg, file, line);

  // Call indirectly to solve generation ordering problem
  Address a(O7, (address)StubRoutines::verify_oop_subroutine_entry_address());

  // Make some space on stack above the current register window.
  // Enough to hold 8 64-bit registers.
  add(SP,-8*8,SP);

  // Save some 64-bit registers; a normal 'save' chops the heads off
  // of 64-bit longs in the 32-bit build.
  stx(O0,SP,frame::register_save_words*wordSize+STACK_BIAS+0*8);
  stx(O1,SP,frame::register_save_words*wordSize+STACK_BIAS+1*8);
  mov(reg,O0); // Move arg into O0; arg might be in O7 which is about to be crushed
  stx(O7,SP,frame::register_save_words*wordSize+STACK_BIAS+7*8);

  set((intptr_t)real_msg, O1);
  // Load address to call to into O7
  load_ptr_contents(a, O7);
  // Register call to verify_oop_subroutine
  callr(O7, G0);
  delayed()->nop();
  // recover frame size
  add(SP, 8*8,SP);
}

void MacroAssembler::_verify_oop_addr(Address addr, const char* msg, const char * file, int line) {
  // plausibility check for oops
  if (!VerifyOops) return;

  char buffer[64];
  sprintf(buffer, "%d", line);
  int len = strlen(file) + strlen(msg) + 1 + 4 + strlen(buffer);
  sprintf(buffer, " at SP+%d ", addr.disp());
  len += strlen(buffer);
  char * real_msg = new char[len];
  sprintf(real_msg, "%s at SP+%d (%s:%d)", msg, addr.disp(), file, line);

  // Call indirectly to solve generation ordering problem
  Address a(O7, (address)StubRoutines::verify_oop_subroutine_entry_address());

  // Make some space on stack above the current register window.
  // Enough to hold 8 64-bit registers.
  add(SP,-8*8,SP);

  // Save some 64-bit registers; a normal 'save' chops the heads off
  // of 64-bit longs in the 32-bit build.
  stx(O0,SP,frame::register_save_words*wordSize+STACK_BIAS+0*8);
  stx(O1,SP,frame::register_save_words*wordSize+STACK_BIAS+1*8);
  ld_ptr(addr.base(), addr.disp() + 8*8, O0); // Load arg into O0; arg might be in O7 which is about to be crushed
  stx(O7,SP,frame::register_save_words*wordSize+STACK_BIAS+7*8);

  set((intptr_t)real_msg, O1);
  // Load address to call to into O7
  load_ptr_contents(a, O7);
  // Register call to verify_oop_subroutine
  callr(O7, G0);
  delayed()->nop();
  // recover frame size
  add(SP, 8*8,SP);
}

// side-door communication with signalHandler in os_solaris.cpp
address MacroAssembler::_verify_oop_implicit_branch[3] = { NULL };

// This macro is expanded just once; it creates shared code.  Contract:
// receives an oop in O0.  Must restore O0 & O7 from TLS.  Must not smash ANY
// registers, including flags.  May not use a register 'save', as this blows
// the high bits of the O-regs if they contain Long values.  Acts as a 'leaf'
// call.
void MacroAssembler::verify_oop_subroutine() {
  assert( VM_Version::v9_instructions_work(), "VerifyOops not supported for V8" );

  // Leaf call; no frame.
  Label succeed, fail, null_or_fail;

  // O0 and O7 were saved already (O0 in O0's TLS home, O7 in O5's TLS home).
  // O0 is now the oop to be checked.  O7 is the return address.
  Register O0_obj = O0;

  // Save some more registers for temps.
  stx(O2,SP,frame::register_save_words*wordSize+STACK_BIAS+2*8);
  stx(O3,SP,frame::register_save_words*wordSize+STACK_BIAS+3*8);
  stx(O4,SP,frame::register_save_words*wordSize+STACK_BIAS+4*8);
  stx(O5,SP,frame::register_save_words*wordSize+STACK_BIAS+5*8);

  // Save flags
  Register O5_save_flags = O5;
  rdccr( O5_save_flags );

  { // count number of verifies
    Register O2_adr   = O2;
    Register O3_accum = O3;
    Address count_addr( O2_adr, (address) StubRoutines::verify_oop_count_addr() );
    sethi(count_addr);
    ld(count_addr, O3_accum);
    inc(O3_accum);
    st(O3_accum, count_addr);
  }

  Register O2_mask = O2;
  Register O3_bits = O3;
  Register O4_temp = O4;

  // mark lower end of faulting range
  assert(_verify_oop_implicit_branch[0] == NULL, "set once");
  _verify_oop_implicit_branch[0] = pc();

  // We can't check the mark oop because it could be in the process of
  // locking or unlocking while this is running.
  set(Universe::verify_oop_mask (), O2_mask);
  set(Universe::verify_oop_bits (), O3_bits);

  // assert((obj & oop_mask) == oop_bits);
  and3(O0_obj, O2_mask, O4_temp);
  cmp(O4_temp, O3_bits);
  brx(notEqual, false, pn, null_or_fail);
  delayed()->nop();

  if ((NULL_WORD & Universe::verify_oop_mask()) == Universe::verify_oop_bits()) {
    // the null_or_fail case is useless; must test for null separately
    br_null(O0_obj, false, pn, succeed);
    delayed()->nop();
  }

  // Check the klassOop of this object for being in the right area of memory.
  // Cannot do the load in the delay above slot in case O0 is null
  load_klass(O0_obj, O0_obj);
  // assert((klass & klass_mask) == klass_bits);
  if( Universe::verify_klass_mask() != Universe::verify_oop_mask() )
    set(Universe::verify_klass_mask(), O2_mask);
  if( Universe::verify_klass_bits() != Universe::verify_oop_bits() )
    set(Universe::verify_klass_bits(), O3_bits);
  and3(O0_obj, O2_mask, O4_temp);
  cmp(O4_temp, O3_bits);
  brx(notEqual, false, pn, fail);
  delayed()->nop();
  // Check the klass's klass
  load_klass(O0_obj, O0_obj);
  and3(O0_obj, O2_mask, O4_temp);
  cmp(O4_temp, O3_bits);
  brx(notEqual, false, pn, fail);
  delayed()->wrccr( O5_save_flags ); // Restore CCR's

  // mark upper end of faulting range
  _verify_oop_implicit_branch[1] = pc();

  //-----------------------
  // all tests pass
  bind(succeed);

  // Restore prior 64-bit registers
  ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+0*8,O0);
  ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+1*8,O1);
  ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+2*8,O2);
  ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+3*8,O3);
  ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+4*8,O4);
  ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+5*8,O5);

  retl();                       // Leaf return; restore prior O7 in delay slot
  delayed()->ldx(SP,frame::register_save_words*wordSize+STACK_BIAS+7*8,O7);

  //-----------------------
  bind(null_or_fail);           // nulls are less common but OK
  br_null(O0_obj, false, pt, succeed);
  delayed()->wrccr( O5_save_flags ); // Restore CCR's

  //-----------------------
  // report failure:
  bind(fail);
  _verify_oop_implicit_branch[2] = pc();

  wrccr( O5_save_flags ); // Restore CCR's

  save_frame(::round_to(sizeof(RegistersForDebugging) / BytesPerWord, 2));

  // stop_subroutine expects message pointer in I1.
  mov(I1, O1);

  // Restore prior 64-bit registers
  ldx(FP,frame::register_save_words*wordSize+STACK_BIAS+0*8,I0);
  ldx(FP,frame::register_save_words*wordSize+STACK_BIAS+1*8,I1);
  ldx(FP,frame::register_save_words*wordSize+STACK_BIAS+2*8,I2);
  ldx(FP,frame::register_save_words*wordSize+STACK_BIAS+3*8,I3);
  ldx(FP,frame::register_save_words*wordSize+STACK_BIAS+4*8,I4);
  ldx(FP,frame::register_save_words*wordSize+STACK_BIAS+5*8,I5);

  // factor long stop-sequence into subroutine to save space
  assert(StubRoutines::Sparc::stop_subroutine_entry_address(), "hasn't been generated yet");

  // call indirectly to solve generation ordering problem
  Address a(O5, (address)StubRoutines::Sparc::stop_subroutine_entry_address());
  load_ptr_contents(a, O5);
  jmpl(O5, 0, O7);
  delayed()->nop();
}


void MacroAssembler::stop(const char* msg) {
  // save frame first to get O7 for return address
  // add one word to size in case struct is odd number of words long
  // It must be doubleword-aligned for storing doubles into it.

    save_frame(::round_to(sizeof(RegistersForDebugging) / BytesPerWord, 2));

    // stop_subroutine expects message pointer in I1.
    set((intptr_t)msg, O1);

    // factor long stop-sequence into subroutine to save space
    assert(StubRoutines::Sparc::stop_subroutine_entry_address(), "hasn't been generated yet");

    // call indirectly to solve generation ordering problem
    Address a(O5, (address)StubRoutines::Sparc::stop_subroutine_entry_address());
    load_ptr_contents(a, O5);
    jmpl(O5, 0, O7);
    delayed()->nop();

    breakpoint_trap();   // make stop actually stop rather than writing
                         // unnoticeable results in the output files.

    // restore(); done in callee to save space!
}


void MacroAssembler::warn(const char* msg) {
  save_frame(::round_to(sizeof(RegistersForDebugging) / BytesPerWord, 2));
  RegistersForDebugging::save_registers(this);
  mov(O0, L0);
  set((intptr_t)msg, O0);
  call( CAST_FROM_FN_PTR(address, warning) );
  delayed()->nop();
//  ret();
//  delayed()->restore();
  RegistersForDebugging::restore_registers(this, L0);
  restore();
}


void MacroAssembler::untested(const char* what) {
  // We must be able to turn interactive prompting off
  // in order to run automated test scripts on the VM
  // Use the flag ShowMessageBoxOnError

  char* b = new char[1024];
  sprintf(b, "untested: %s", what);

  if ( ShowMessageBoxOnError )   stop(b);
  else                           warn(b);
}


void MacroAssembler::stop_subroutine() {
  RegistersForDebugging::save_registers(this);

  // for the sake of the debugger, stick a PC on the current frame
  // (this assumes that the caller has performed an extra "save")
  mov(I7, L7);
  add(O7, -7 * BytesPerInt, I7);

  save_frame(); // one more save to free up another O7 register
  mov(I0, O1); // addr of reg save area

  // We expect pointer to message in I1. Caller must set it up in O1
  mov(I1, O0); // get msg
  call (CAST_FROM_FN_PTR(address, MacroAssembler::debug), relocInfo::runtime_call_type);
  delayed()->nop();

  restore();

  RegistersForDebugging::restore_registers(this, O0);

  save_frame(0);
  call(CAST_FROM_FN_PTR(address,breakpoint));
  delayed()->nop();
  restore();

  mov(L7, I7);
  retl();
  delayed()->restore(); // see stop above
}


void MacroAssembler::debug(char* msg, RegistersForDebugging* regs) {
  if ( ShowMessageBoxOnError ) {
      JavaThreadState saved_state = JavaThread::current()->thread_state();
      JavaThread::current()->set_thread_state(_thread_in_vm);
      {
        // In order to get locks work, we need to fake a in_VM state
        ttyLocker ttyl;
        ::tty->print_cr("EXECUTION STOPPED: %s\n", msg);
        if (CountBytecodes || TraceBytecodes || StopInterpreterAt) {
          ::tty->print_cr("Interpreter::bytecode_counter = %d", BytecodeCounter::counter_value());
        }
        if (os::message_box(msg, "Execution stopped, print registers?"))
          regs->print(::tty);
      }
      ThreadStateTransition::transition(JavaThread::current(), _thread_in_vm, saved_state);
  }
  else
     ::tty->print_cr("=============== DEBUG MESSAGE: %s ================\n", msg);
  assert(false, "error");
}


#ifndef PRODUCT
void MacroAssembler::test() {
  ResourceMark rm;

  CodeBuffer cb("test", 10000, 10000);
  MacroAssembler* a = new MacroAssembler(&cb);
  VM_Version::allow_all();
  a->test_v9();
  a->test_v8_onlys();
  VM_Version::revert();

  StubRoutines::Sparc::test_stop_entry()();
}
#endif


void MacroAssembler::calc_mem_param_words(Register Rparam_words, Register Rresult) {
  subcc( Rparam_words, Argument::n_register_parameters, Rresult); // how many mem words?
  Label no_extras;
  br( negative, true, pt, no_extras ); // if neg, clear reg
  delayed()->set( 0, Rresult);         // annuled, so only if taken
  bind( no_extras );
}


void MacroAssembler::calc_frame_size(Register Rextra_words, Register Rresult) {
#ifdef _LP64
  add(Rextra_words, frame::memory_parameter_word_sp_offset, Rresult);
#else
  add(Rextra_words, frame::memory_parameter_word_sp_offset + 1, Rresult);
#endif
  bclr(1, Rresult);
  sll(Rresult, LogBytesPerWord, Rresult);  // Rresult has total frame bytes
}


void MacroAssembler::calc_frame_size_and_save(Register Rextra_words, Register Rresult) {
  calc_frame_size(Rextra_words, Rresult);
  neg(Rresult);
  save(SP, Rresult, SP);
}


// ---------------------------------------------------------
Assembler::RCondition cond2rcond(Assembler::Condition c) {
  switch (c) {
    /*case zero: */
    case Assembler::equal:        return Assembler::rc_z;
    case Assembler::lessEqual:    return Assembler::rc_lez;
    case Assembler::less:         return Assembler::rc_lz;
    /*case notZero:*/
    case Assembler::notEqual:     return Assembler::rc_nz;
    case Assembler::greater:      return Assembler::rc_gz;
    case Assembler::greaterEqual: return Assembler::rc_gez;
  }
  ShouldNotReachHere();
  return Assembler::rc_z;
}

// compares register with zero and branches.  NOT FOR USE WITH 64-bit POINTERS
void MacroAssembler::br_zero( Condition c, bool a, Predict p, Register s1, Label& L) {
  tst(s1);
  br (c, a, p, L);
}


// Compares a pointer register with zero and branches on null.
// Does a test & branch on 32-bit systems and a register-branch on 64-bit.
void MacroAssembler::br_null( Register s1, bool a, Predict p, Label& L ) {
  assert_not_delayed();
#ifdef _LP64
  bpr( rc_z, a, p, s1, L );
#else
  tst(s1);
  br ( zero, a, p, L );
#endif
}

void MacroAssembler::br_notnull( Register s1, bool a, Predict p, Label& L ) {
  assert_not_delayed();
#ifdef _LP64
  bpr( rc_nz, a, p, s1, L );
#else
  tst(s1);
  br ( notZero, a, p, L );
#endif
}


// instruction sequences factored across compiler & interpreter


void MacroAssembler::lcmp( Register Ra_hi, Register Ra_low,
                           Register Rb_hi, Register Rb_low,
                           Register Rresult) {

  Label check_low_parts, done;

  cmp(Ra_hi, Rb_hi );  // compare hi parts
  br(equal, true, pt, check_low_parts);
  delayed()->cmp(Ra_low, Rb_low); // test low parts

  // And, with an unsigned comparison, it does not matter if the numbers
  // are negative or not.
  // E.g., -2 cmp -1: the low parts are 0xfffffffe and 0xffffffff.
  // The second one is bigger (unsignedly).

  // Other notes:  The first move in each triplet can be unconditional
  // (and therefore probably prefetchable).
  // And the equals case for the high part does not need testing,
  // since that triplet is reached only after finding the high halves differ.

  if (VM_Version::v9_instructions_work()) {

                                    mov  (                     -1, Rresult);
    ba( false, done );  delayed()-> movcc(greater, false, icc,  1, Rresult);
  }
  else {
    br(less,    true, pt, done); delayed()-> set(-1, Rresult);
    br(greater, true, pt, done); delayed()-> set( 1, Rresult);
  }

  bind( check_low_parts );

  if (VM_Version::v9_instructions_work()) {
    mov(                               -1, Rresult);
    movcc(equal,           false, icc,  0, Rresult);
    movcc(greaterUnsigned, false, icc,  1, Rresult);
  }
  else {
                                                    set(-1, Rresult);
    br(equal,           true, pt, done); delayed()->set( 0, Rresult);
    br(greaterUnsigned, true, pt, done); delayed()->set( 1, Rresult);
  }
  bind( done );
}

void MacroAssembler::lneg( Register Rhi, Register Rlow ) {
  subcc(  G0, Rlow, Rlow );
  subc(   G0, Rhi,  Rhi  );
}

void MacroAssembler::lshl( Register Rin_high,  Register Rin_low,
                           Register Rcount,
                           Register Rout_high, Register Rout_low,
                           Register Rtemp ) {


  Register Ralt_count = Rtemp;
  Register Rxfer_bits = Rtemp;

  assert( Ralt_count != Rin_high
      &&  Ralt_count != Rin_low
      &&  Ralt_count != Rcount
      &&  Rxfer_bits != Rin_low
      &&  Rxfer_bits != Rin_high
      &&  Rxfer_bits != Rcount
      &&  Rxfer_bits != Rout_low
      &&  Rout_low   != Rin_high,
        "register alias checks");

  Label big_shift, done;

  // This code can be optimized to use the 64 bit shifts in V9.
  // Here we use the 32 bit shifts.

  and3( Rcount,         0x3f,           Rcount);     // take least significant 6 bits
  subcc(Rcount,         31,             Ralt_count);
  br(greater, true, pn, big_shift);
  delayed()->
  dec(Ralt_count);

  // shift < 32 bits, Ralt_count = Rcount-31

  // We get the transfer bits by shifting right by 32-count the low
  // register. This is done by shifting right by 31-count and then by one
  // more to take care of the special (rare) case where count is zero
  // (shifting by 32 would not work).

  neg(  Ralt_count                                 );

  // The order of the next two instructions is critical in the case where
  // Rin and Rout are the same and should not be reversed.

  srl(  Rin_low,        Ralt_count,     Rxfer_bits ); // shift right by 31-count
  if (Rcount != Rout_low) {
    sll(        Rin_low,        Rcount,         Rout_low   ); // low half
  }
  sll(  Rin_high,       Rcount,         Rout_high  );
  if (Rcount == Rout_low) {
    sll(        Rin_low,        Rcount,         Rout_low   ); // low half
  }
  srl(  Rxfer_bits,     1,              Rxfer_bits ); // shift right by one more
  ba (false, done);
  delayed()->
  or3(  Rout_high,      Rxfer_bits,     Rout_high);   // new hi value: or in shifted old hi part and xfer from low

  // shift >= 32 bits, Ralt_count = Rcount-32
  bind(big_shift);
  sll(  Rin_low,        Ralt_count,     Rout_high  );
  clr(  Rout_low                                   );

  bind(done);
}


void MacroAssembler::lshr( Register Rin_high,  Register Rin_low,
                           Register Rcount,
                           Register Rout_high, Register Rout_low,
                           Register Rtemp ) {

  Register Ralt_count = Rtemp;
  Register Rxfer_bits = Rtemp;

  assert( Ralt_count != Rin_high
      &&  Ralt_count != Rin_low
      &&  Ralt_count != Rcount
      &&  Rxfer_bits != Rin_low
      &&  Rxfer_bits != Rin_high
      &&  Rxfer_bits != Rcount
      &&  Rxfer_bits != Rout_high
      &&  Rout_high  != Rin_low,
        "register alias checks");

  Label big_shift, done;

  // This code can be optimized to use the 64 bit shifts in V9.
  // Here we use the 32 bit shifts.

  and3( Rcount,         0x3f,           Rcount);     // take least significant 6 bits
  subcc(Rcount,         31,             Ralt_count);
  br(greater, true, pn, big_shift);
  delayed()->dec(Ralt_count);

  // shift < 32 bits, Ralt_count = Rcount-31

  // We get the transfer bits by shifting left by 32-count the high
  // register. This is done by shifting left by 31-count and then by one
  // more to take care of the special (rare) case where count is zero
  // (shifting by 32 would not work).

  neg(  Ralt_count                                  );
  if (Rcount != Rout_low) {
    srl(        Rin_low,        Rcount,         Rout_low    );
  }

  // The order of the next two instructions is critical in the case where
  // Rin and Rout are the same and should not be reversed.

  sll(  Rin_high,       Ralt_count,     Rxfer_bits  ); // shift left by 31-count
  sra(  Rin_high,       Rcount,         Rout_high   ); // high half
  sll(  Rxfer_bits,     1,              Rxfer_bits  ); // shift left by one more
  if (Rcount == Rout_low) {
    srl(        Rin_low,        Rcount,         Rout_low    );
  }
  ba (false, done);
  delayed()->
  or3(  Rout_low,       Rxfer_bits,     Rout_low    ); // new low value: or shifted old low part and xfer from high

  // shift >= 32 bits, Ralt_count = Rcount-32
  bind(big_shift);

  sra(  Rin_high,       Ralt_count,     Rout_low    );
  sra(  Rin_high,       31,             Rout_high   ); // sign into hi

  bind( done );
}



void MacroAssembler::lushr( Register Rin_high,  Register Rin_low,
                            Register Rcount,
                            Register Rout_high, Register Rout_low,
                            Register Rtemp ) {

  Register Ralt_count = Rtemp;
  Register Rxfer_bits = Rtemp;

  assert( Ralt_count != Rin_high
      &&  Ralt_count != Rin_low
      &&  Ralt_count != Rcount
      &&  Rxfer_bits != Rin_low
      &&  Rxfer_bits != Rin_high
      &&  Rxfer_bits != Rcount
      &&  Rxfer_bits != Rout_high
      &&  Rout_high  != Rin_low,
        "register alias checks");

  Label big_shift, done;

  // This code can be optimized to use the 64 bit shifts in V9.
  // Here we use the 32 bit shifts.

  and3( Rcount,         0x3f,           Rcount);     // take least significant 6 bits
  subcc(Rcount,         31,             Ralt_count);
  br(greater, true, pn, big_shift);
  delayed()->dec(Ralt_count);

  // shift < 32 bits, Ralt_count = Rcount-31

  // We get the transfer bits by shifting left by 32-count the high
  // register. This is done by shifting left by 31-count and then by one
  // more to take care of the special (rare) case where count is zero
  // (shifting by 32 would not work).

  neg(  Ralt_count                                  );
  if (Rcount != Rout_low) {
    srl(        Rin_low,        Rcount,         Rout_low    );
  }

  // The order of the next two instructions is critical in the case where
  // Rin and Rout are the same and should not be reversed.

  sll(  Rin_high,       Ralt_count,     Rxfer_bits  ); // shift left by 31-count
  srl(  Rin_high,       Rcount,         Rout_high   ); // high half
  sll(  Rxfer_bits,     1,              Rxfer_bits  ); // shift left by one more
  if (Rcount == Rout_low) {
    srl(        Rin_low,        Rcount,         Rout_low    );
  }
  ba (false, done);
  delayed()->
  or3(  Rout_low,       Rxfer_bits,     Rout_low    ); // new low value: or shifted old low part and xfer from high

  // shift >= 32 bits, Ralt_count = Rcount-32
  bind(big_shift);

  srl(  Rin_high,       Ralt_count,     Rout_low    );
  clr(  Rout_high                                   );

  bind( done );
}

#ifdef _LP64
void MacroAssembler::lcmp( Register Ra, Register Rb, Register Rresult) {
  cmp(Ra, Rb);
  mov(                       -1, Rresult);
  movcc(equal,   false, xcc,  0, Rresult);
  movcc(greater, false, xcc,  1, Rresult);
}
#endif


void MacroAssembler::float_cmp( bool is_float, int unordered_result,
                                FloatRegister Fa, FloatRegister Fb,
                                Register Rresult) {

  fcmp(is_float ? FloatRegisterImpl::S : FloatRegisterImpl::D, fcc0, Fa, Fb);

  Condition lt = unordered_result == -1 ? f_unorderedOrLess    : f_less;
  Condition eq =                          f_equal;
  Condition gt = unordered_result ==  1 ? f_unorderedOrGreater : f_greater;

  if (VM_Version::v9_instructions_work()) {

    mov(                   -1, Rresult );
    movcc( eq, true, fcc0,  0, Rresult );
    movcc( gt, true, fcc0,  1, Rresult );

  } else {
    Label done;

                                         set( -1, Rresult );
    //fb(lt, true, pn, done); delayed()->set( -1, Rresult );
    fb( eq, true, pn, done);  delayed()->set(  0, Rresult );
    fb( gt, true, pn, done);  delayed()->set(  1, Rresult );

    bind (done);
  }
}


void MacroAssembler::fneg( FloatRegisterImpl::Width w, FloatRegister s, FloatRegister d)
{
  if (VM_Version::v9_instructions_work()) {
    Assembler::fneg(w, s, d);
  } else {
    if (w == FloatRegisterImpl::S) {
      Assembler::fneg(w, s, d);
    } else if (w == FloatRegisterImpl::D) {
      // number() does a sanity check on the alignment.
      assert(((s->encoding(FloatRegisterImpl::D) & 1) == 0) &&
        ((d->encoding(FloatRegisterImpl::D) & 1) == 0), "float register alignment check");

      Assembler::fneg(FloatRegisterImpl::S, s, d);
      Assembler::fmov(FloatRegisterImpl::S, s->successor(), d->successor());
    } else {
      assert(w == FloatRegisterImpl::Q, "Invalid float register width");

      // number() does a sanity check on the alignment.
      assert(((s->encoding(FloatRegisterImpl::D) & 3) == 0) &&
        ((d->encoding(FloatRegisterImpl::D) & 3) == 0), "float register alignment check");

      Assembler::fneg(FloatRegisterImpl::S, s, d);
      Assembler::fmov(FloatRegisterImpl::S, s->successor(), d->successor());
      Assembler::fmov(FloatRegisterImpl::S, s->successor()->successor(), d->successor()->successor());
      Assembler::fmov(FloatRegisterImpl::S, s->successor()->successor()->successor(), d->successor()->successor()->successor());
    }
  }
}

void MacroAssembler::fmov( FloatRegisterImpl::Width w, FloatRegister s, FloatRegister d)
{
  if (VM_Version::v9_instructions_work()) {
    Assembler::fmov(w, s, d);
  } else {
    if (w == FloatRegisterImpl::S) {
      Assembler::fmov(w, s, d);
    } else if (w == FloatRegisterImpl::D) {
      // number() does a sanity check on the alignment.
      assert(((s->encoding(FloatRegisterImpl::D) & 1) == 0) &&
        ((d->encoding(FloatRegisterImpl::D) & 1) == 0), "float register alignment check");

      Assembler::fmov(FloatRegisterImpl::S, s, d);
      Assembler::fmov(FloatRegisterImpl::S, s->successor(), d->successor());
    } else {
      assert(w == FloatRegisterImpl::Q, "Invalid float register width");

      // number() does a sanity check on the alignment.
      assert(((s->encoding(FloatRegisterImpl::D) & 3) == 0) &&
        ((d->encoding(FloatRegisterImpl::D) & 3) == 0), "float register alignment check");

      Assembler::fmov(FloatRegisterImpl::S, s, d);
      Assembler::fmov(FloatRegisterImpl::S, s->successor(), d->successor());
      Assembler::fmov(FloatRegisterImpl::S, s->successor()->successor(), d->successor()->successor());
      Assembler::fmov(FloatRegisterImpl::S, s->successor()->successor()->successor(), d->successor()->successor()->successor());
    }
  }
}

void MacroAssembler::fabs( FloatRegisterImpl::Width w, FloatRegister s, FloatRegister d)
{
  if (VM_Version::v9_instructions_work()) {
    Assembler::fabs(w, s, d);
  } else {
    if (w == FloatRegisterImpl::S) {
      Assembler::fabs(w, s, d);
    } else if (w == FloatRegisterImpl::D) {
      // number() does a sanity check on the alignment.
      assert(((s->encoding(FloatRegisterImpl::D) & 1) == 0) &&
        ((d->encoding(FloatRegisterImpl::D) & 1) == 0), "float register alignment check");

      Assembler::fabs(FloatRegisterImpl::S, s, d);
      Assembler::fmov(FloatRegisterImpl::S, s->successor(), d->successor());
    } else {
      assert(w == FloatRegisterImpl::Q, "Invalid float register width");

      // number() does a sanity check on the alignment.
      assert(((s->encoding(FloatRegisterImpl::D) & 3) == 0) &&
       ((d->encoding(FloatRegisterImpl::D) & 3) == 0), "float register alignment check");

      Assembler::fabs(FloatRegisterImpl::S, s, d);
      Assembler::fmov(FloatRegisterImpl::S, s->successor(), d->successor());
      Assembler::fmov(FloatRegisterImpl::S, s->successor()->successor(), d->successor()->successor());
      Assembler::fmov(FloatRegisterImpl::S, s->successor()->successor()->successor(), d->successor()->successor()->successor());
    }
  }
}

void MacroAssembler::save_all_globals_into_locals() {
  mov(G1,L1);
  mov(G2,L2);
  mov(G3,L3);
  mov(G4,L4);
  mov(G5,L5);
  mov(G6,L6);
  mov(G7,L7);
}

void MacroAssembler::restore_globals_from_locals() {
  mov(L1,G1);
  mov(L2,G2);
  mov(L3,G3);
  mov(L4,G4);
  mov(L5,G5);
  mov(L6,G6);
  mov(L7,G7);
}

// Use for 64 bit operation.
void MacroAssembler::casx_under_lock(Register top_ptr_reg, Register top_reg, Register ptr_reg, address lock_addr, bool use_call_vm)
{
  // store ptr_reg as the new top value
#ifdef _LP64
  casx(top_ptr_reg, top_reg, ptr_reg);
#else
  cas_under_lock(top_ptr_reg, top_reg, ptr_reg, lock_addr, use_call_vm);
#endif // _LP64
}

// [RGV] This routine does not handle 64 bit operations.
//       use casx_under_lock() or casx directly!!!
void MacroAssembler::cas_under_lock(Register top_ptr_reg, Register top_reg, Register ptr_reg, address lock_addr, bool use_call_vm)
{
  // store ptr_reg as the new top value
  if (VM_Version::v9_instructions_work()) {
    cas(top_ptr_reg, top_reg, ptr_reg);
  } else {

    // If the register is not an out nor global, it is not visible
    // after the save.  Allocate a register for it, save its
    // value in the register save area (the save may not flush
    // registers to the save area).

    Register top_ptr_reg_after_save;
    Register top_reg_after_save;
    Register ptr_reg_after_save;

    if (top_ptr_reg->is_out() || top_ptr_reg->is_global()) {
      top_ptr_reg_after_save = top_ptr_reg->after_save();
    } else {
      Address reg_save_addr = top_ptr_reg->address_in_saved_window();
      top_ptr_reg_after_save = L0;
      st(top_ptr_reg, reg_save_addr);
    }

    if (top_reg->is_out() || top_reg->is_global()) {
      top_reg_after_save = top_reg->after_save();
    } else {
      Address reg_save_addr = top_reg->address_in_saved_window();
      top_reg_after_save = L1;
      st(top_reg, reg_save_addr);
    }

    if (ptr_reg->is_out() || ptr_reg->is_global()) {
      ptr_reg_after_save = ptr_reg->after_save();
    } else {
      Address reg_save_addr = ptr_reg->address_in_saved_window();
      ptr_reg_after_save = L2;
      st(ptr_reg, reg_save_addr);
    }

    const Register& lock_reg = L3;
    const Register& lock_ptr_reg = L4;
    const Register& value_reg = L5;
    const Register& yield_reg = L6;
    const Register& yieldall_reg = L7;

    save_frame();

    if (top_ptr_reg_after_save == L0) {
      ld(top_ptr_reg->address_in_saved_window().after_save(), top_ptr_reg_after_save);
    }

    if (top_reg_after_save == L1) {
      ld(top_reg->address_in_saved_window().after_save(), top_reg_after_save);
    }

    if (ptr_reg_after_save == L2) {
      ld(ptr_reg->address_in_saved_window().after_save(), ptr_reg_after_save);
    }

    Label(retry_get_lock);
    Label(not_same);
    Label(dont_yield);

    assert(lock_addr, "lock_address should be non null for v8");
    set((intptr_t)lock_addr, lock_ptr_reg);
    // Initialize yield counter
    mov(G0,yield_reg);
    mov(G0, yieldall_reg);
    set(StubRoutines::Sparc::locked, lock_reg);

    bind(retry_get_lock);
    cmp(yield_reg, V8AtomicOperationUnderLockSpinCount);
    br(Assembler::less, false, Assembler::pt, dont_yield);
    delayed()->nop();

    if(use_call_vm) {
      Untested("Need to verify global reg consistancy");
      call_VM(noreg, CAST_FROM_FN_PTR(address, SharedRuntime::yield_all), yieldall_reg);
    } else {
      // Save the regs and make space for a C call
      save(SP, -96, SP);
      save_all_globals_into_locals();
      call(CAST_FROM_FN_PTR(address,os::yield_all));
      delayed()->mov(yieldall_reg, O0);
      restore_globals_from_locals();
      restore();
    }

    // reset the counter
    mov(G0,yield_reg);
    add(yieldall_reg, 1, yieldall_reg);

    bind(dont_yield);
    // try to get lock
    swap(lock_ptr_reg, 0, lock_reg);

    // did we get the lock?
    cmp(lock_reg, StubRoutines::Sparc::unlocked);
    br(Assembler::notEqual, true, Assembler::pn, retry_get_lock);
    delayed()->add(yield_reg,1,yield_reg);

    // yes, got lock.  do we have the same top?
    ld(top_ptr_reg_after_save, 0, value_reg);
    cmp(value_reg, top_reg_after_save);
    br(Assembler::notEqual, false, Assembler::pn, not_same);
    delayed()->nop();

    // yes, same top.
    st(ptr_reg_after_save, top_ptr_reg_after_save, 0);
    membar(Assembler::StoreStore);

    bind(not_same);
    mov(value_reg, ptr_reg_after_save);
    st(lock_reg, lock_ptr_reg, 0); // unlock

    restore();
  }
}

void MacroAssembler::biased_locking_enter(Register obj_reg, Register mark_reg, Register temp_reg,
                                          Label& done, Label* slow_case,
                                          BiasedLockingCounters* counters) {
  assert(UseBiasedLocking, "why call this otherwise?");

  if (PrintBiasedLockingStatistics) {
    assert_different_registers(obj_reg, mark_reg, temp_reg, O7);
    if (counters == NULL)
      counters = BiasedLocking::counters();
  }

  Label cas_label;

  // Biased locking
  // See whether the lock is currently biased toward our thread and
  // whether the epoch is still valid
  // Note that the runtime guarantees sufficient alignment of JavaThread
  // pointers to allow age to be placed into low bits
  assert(markOopDesc::age_shift == markOopDesc::lock_bits + markOopDesc::biased_lock_bits, "biased locking makes assumptions about bit layout");
  and3(mark_reg, markOopDesc::biased_lock_mask_in_place, temp_reg);
  cmp(temp_reg, markOopDesc::biased_lock_pattern);
  brx(Assembler::notEqual, false, Assembler::pn, cas_label);
  delayed()->nop();

  load_klass(obj_reg, temp_reg);
  ld_ptr(Address(temp_reg, 0, Klass::prototype_header_offset_in_bytes() + klassOopDesc::klass_part_offset_in_bytes()), temp_reg);
  or3(G2_thread, temp_reg, temp_reg);
  xor3(mark_reg, temp_reg, temp_reg);
  andcc(temp_reg, ~((int) markOopDesc::age_mask_in_place), temp_reg);
  if (counters != NULL) {
    cond_inc(Assembler::equal, (address) counters->biased_lock_entry_count_addr(), mark_reg, temp_reg);
    // Reload mark_reg as we may need it later
    ld_ptr(Address(obj_reg, 0, oopDesc::mark_offset_in_bytes()), mark_reg);
  }
  brx(Assembler::equal, true, Assembler::pt, done);
  delayed()->nop();

  Label try_revoke_bias;
  Label try_rebias;
  Address mark_addr = Address(obj_reg, 0, oopDesc::mark_offset_in_bytes());
  assert(mark_addr.disp() == 0, "cas must take a zero displacement");

  // At this point we know that the header has the bias pattern and
  // that we are not the bias owner in the current epoch. We need to
  // figure out more details about the state of the header in order to
  // know what operations can be legally performed on the object's
  // header.

  // If the low three bits in the xor result aren't clear, that means
  // the prototype header is no longer biased and we have to revoke
  // the bias on this object.
  btst(markOopDesc::biased_lock_mask_in_place, temp_reg);
  brx(Assembler::notZero, false, Assembler::pn, try_revoke_bias);

  // Biasing is still enabled for this data type. See whether the
  // epoch of the current bias is still valid, meaning that the epoch
  // bits of the mark word are equal to the epoch bits of the
  // prototype header. (Note that the prototype header's epoch bits
  // only change at a safepoint.) If not, attempt to rebias the object
  // toward the current thread. Note that we must be absolutely sure
  // that the current epoch is invalid in order to do this because
  // otherwise the manipulations it performs on the mark word are
  // illegal.
  delayed()->btst(markOopDesc::epoch_mask_in_place, temp_reg);
  brx(Assembler::notZero, false, Assembler::pn, try_rebias);

  // The epoch of the current bias is still valid but we know nothing
  // about the owner; it might be set or it might be clear. Try to
  // acquire the bias of the object using an atomic operation. If this
  // fails we will go in to the runtime to revoke the object's bias.
  // Note that we first construct the presumed unbiased header so we
  // don't accidentally blow away another thread's valid bias.
  delayed()->and3(mark_reg,
                  markOopDesc::biased_lock_mask_in_place | markOopDesc::age_mask_in_place | markOopDesc::epoch_mask_in_place,
                  mark_reg);
  or3(G2_thread, mark_reg, temp_reg);
  casx_under_lock(mark_addr.base(), mark_reg, temp_reg,
                  (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr());
  // If the biasing toward our thread failed, this means that
  // another thread succeeded in biasing it toward itself and we
  // need to revoke that bias. The revocation will occur in the
  // interpreter runtime in the slow case.
  cmp(mark_reg, temp_reg);
  if (counters != NULL) {
    cond_inc(Assembler::zero, (address) counters->anonymously_biased_lock_entry_count_addr(), mark_reg, temp_reg);
  }
  if (slow_case != NULL) {
    brx(Assembler::notEqual, true, Assembler::pn, *slow_case);
    delayed()->nop();
  }
  br(Assembler::always, false, Assembler::pt, done);
  delayed()->nop();

  bind(try_rebias);
  // At this point we know the epoch has expired, meaning that the
  // current "bias owner", if any, is actually invalid. Under these
  // circumstances _only_, we are allowed to use the current header's
  // value as the comparison value when doing the cas to acquire the
  // bias in the current epoch. In other words, we allow transfer of
  // the bias from one thread to another directly in this situation.
  //
  // FIXME: due to a lack of registers we currently blow away the age
  // bits in this situation. Should attempt to preserve them.
  load_klass(obj_reg, temp_reg);
  ld_ptr(Address(temp_reg, 0, Klass::prototype_header_offset_in_bytes() + klassOopDesc::klass_part_offset_in_bytes()), temp_reg);
  or3(G2_thread, temp_reg, temp_reg);
  casx_under_lock(mark_addr.base(), mark_reg, temp_reg,
                  (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr());
  // If the biasing toward our thread failed, this means that
  // another thread succeeded in biasing it toward itself and we
  // need to revoke that bias. The revocation will occur in the
  // interpreter runtime in the slow case.
  cmp(mark_reg, temp_reg);
  if (counters != NULL) {
    cond_inc(Assembler::zero, (address) counters->rebiased_lock_entry_count_addr(), mark_reg, temp_reg);
  }
  if (slow_case != NULL) {
    brx(Assembler::notEqual, true, Assembler::pn, *slow_case);
    delayed()->nop();
  }
  br(Assembler::always, false, Assembler::pt, done);
  delayed()->nop();

  bind(try_revoke_bias);
  // The prototype mark in the klass doesn't have the bias bit set any
  // more, indicating that objects of this data type are not supposed
  // to be biased any more. We are going to try to reset the mark of
  // this object to the prototype value and fall through to the
  // CAS-based locking scheme. Note that if our CAS fails, it means
  // that another thread raced us for the privilege of revoking the
  // bias of this particular object, so it's okay to continue in the
  // normal locking code.
  //
  // FIXME: due to a lack of registers we currently blow away the age
  // bits in this situation. Should attempt to preserve them.
  load_klass(obj_reg, temp_reg);
  ld_ptr(Address(temp_reg, 0, Klass::prototype_header_offset_in_bytes() + klassOopDesc::klass_part_offset_in_bytes()), temp_reg);
  casx_under_lock(mark_addr.base(), mark_reg, temp_reg,
                  (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr());
  // Fall through to the normal CAS-based lock, because no matter what
  // the result of the above CAS, some thread must have succeeded in
  // removing the bias bit from the object's header.
  if (counters != NULL) {
    cmp(mark_reg, temp_reg);
    cond_inc(Assembler::zero, (address) counters->revoked_lock_entry_count_addr(), mark_reg, temp_reg);
  }

  bind(cas_label);
}

void MacroAssembler::biased_locking_exit (Address mark_addr, Register temp_reg, Label& done,
                                          bool allow_delay_slot_filling) {
  // Check for biased locking unlock case, which is a no-op
  // Note: we do not have to check the thread ID for two reasons.
  // First, the interpreter checks for IllegalMonitorStateException at
  // a higher level. Second, if the bias was revoked while we held the
  // lock, the object could not be rebiased toward another thread, so
  // the bias bit would be clear.
  ld_ptr(mark_addr, temp_reg);
  and3(temp_reg, markOopDesc::biased_lock_mask_in_place, temp_reg);
  cmp(temp_reg, markOopDesc::biased_lock_pattern);
  brx(Assembler::equal, allow_delay_slot_filling, Assembler::pt, done);
  delayed();
  if (!allow_delay_slot_filling) {
    nop();
  }
}


// CASN -- 32-64 bit switch hitter similar to the synthetic CASN provided by
// Solaris/SPARC's "as".  Another apt name would be cas_ptr()

void MacroAssembler::casn (Register addr_reg, Register cmp_reg, Register set_reg ) {
  casx_under_lock (addr_reg, cmp_reg, set_reg, (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr()) ;
}



// compiler_lock_object() and compiler_unlock_object() are direct transliterations
// of i486.ad fast_lock() and fast_unlock().  See those methods for detailed comments.
// The code could be tightened up considerably.
//
// box->dhw disposition - post-conditions at DONE_LABEL.
// -   Successful inflated lock:  box->dhw != 0.
//     Any non-zero value suffices.
//     Consider G2_thread, rsp, boxReg, or unused_mark()
// -   Successful Stack-lock: box->dhw == mark.
//     box->dhw must contain the displaced mark word value
// -   Failure -- icc.ZFlag == 0 and box->dhw is undefined.
//     The slow-path fast_enter() and slow_enter() operators
//     are responsible for setting box->dhw = NonZero (typically ::unused_mark).
// -   Biased: box->dhw is undefined
//
// SPARC refworkload performance - specifically jetstream and scimark - are
// extremely sensitive to the size of the code emitted by compiler_lock_object
// and compiler_unlock_object.  Critically, the key factor is code size, not path
// length.  (Simply experiments to pad CLO with unexecuted NOPs demonstrte the
// effect).


void MacroAssembler::compiler_lock_object(Register Roop, Register Rmark, Register Rbox, Register Rscratch,
                                          BiasedLockingCounters* counters) {
   Address mark_addr(Roop, 0, oopDesc::mark_offset_in_bytes());

   verify_oop(Roop);
   Label done ;

   if (counters != NULL) {
     inc_counter((address) counters->total_entry_count_addr(), Rmark, Rscratch);
   }

   if (EmitSync & 1) {
     mov    (3, Rscratch) ;
     st_ptr (Rscratch, Rbox, BasicLock::displaced_header_offset_in_bytes());
     cmp    (SP, G0) ;
     return ;
   }

   if (EmitSync & 2) {

     // Fetch object's markword
     ld_ptr(mark_addr, Rmark);

     if (UseBiasedLocking) {
        biased_locking_enter(Roop, Rmark, Rscratch, done, NULL, counters);
     }

     // Save Rbox in Rscratch to be used for the cas operation
     mov(Rbox, Rscratch);

     // set Rmark to markOop | markOopDesc::unlocked_value
     or3(Rmark, markOopDesc::unlocked_value, Rmark);

     // Initialize the box.  (Must happen before we update the object mark!)
     st_ptr(Rmark, Rbox, BasicLock::displaced_header_offset_in_bytes());

     // compare object markOop with Rmark and if equal exchange Rscratch with object markOop
     assert(mark_addr.disp() == 0, "cas must take a zero displacement");
     casx_under_lock(mark_addr.base(), Rmark, Rscratch,
        (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr());

     // if compare/exchange succeeded we found an unlocked object and we now have locked it
     // hence we are done
     cmp(Rmark, Rscratch);
#ifdef _LP64
     sub(Rscratch, STACK_BIAS, Rscratch);
#endif
     brx(Assembler::equal, false, Assembler::pt, done);
     delayed()->sub(Rscratch, SP, Rscratch);  //pull next instruction into delay slot

     // we did not find an unlocked object so see if this is a recursive case
     // sub(Rscratch, SP, Rscratch);
     assert(os::vm_page_size() > 0xfff, "page size too small - change the constant");
     andcc(Rscratch, 0xfffff003, Rscratch);
     st_ptr(Rscratch, Rbox, BasicLock::displaced_header_offset_in_bytes());
     bind (done) ;
     return ;
   }

   Label Egress ;

   if (EmitSync & 256) {
      Label IsInflated ;

      ld_ptr (mark_addr, Rmark);           // fetch obj->mark
      // Triage: biased, stack-locked, neutral, inflated
      if (UseBiasedLocking) {
        biased_locking_enter(Roop, Rmark, Rscratch, done, NULL, counters);
        // Invariant: if control reaches this point in the emitted stream
        // then Rmark has not been modified.
      }

      // Store mark into displaced mark field in the on-stack basic-lock "box"
      // Critically, this must happen before the CAS
      // Maximize the ST-CAS distance to minimize the ST-before-CAS penalty.
      st_ptr (Rmark, Rbox, BasicLock::displaced_header_offset_in_bytes());
      andcc  (Rmark, 2, G0) ;
      brx    (Assembler::notZero, false, Assembler::pn, IsInflated) ;
      delayed() ->

      // Try stack-lock acquisition.
      // Beware: the 1st instruction is in a delay slot
      mov    (Rbox,  Rscratch);
      or3    (Rmark, markOopDesc::unlocked_value, Rmark);
      assert (mark_addr.disp() == 0, "cas must take a zero displacement");
      casn   (mark_addr.base(), Rmark, Rscratch) ;
      cmp    (Rmark, Rscratch);
      brx    (Assembler::equal, false, Assembler::pt, done);
      delayed()->sub(Rscratch, SP, Rscratch);

      // Stack-lock attempt failed - check for recursive stack-lock.
      // See the comments below about how we might remove this case.
#ifdef _LP64
      sub    (Rscratch, STACK_BIAS, Rscratch);
#endif
      assert(os::vm_page_size() > 0xfff, "page size too small - change the constant");
      andcc  (Rscratch, 0xfffff003, Rscratch);
      br     (Assembler::always, false, Assembler::pt, done) ;
      delayed()-> st_ptr (Rscratch, Rbox, BasicLock::displaced_header_offset_in_bytes());

      bind   (IsInflated) ;
      if (EmitSync & 64) {
         // If m->owner != null goto IsLocked
         // Pessimistic form: Test-and-CAS vs CAS
         // The optimistic form avoids RTS->RTO cache line upgrades.
         ld_ptr (Address (Rmark, 0, ObjectMonitor::owner_offset_in_bytes()-2), Rscratch) ;
         andcc  (Rscratch, Rscratch, G0) ;
         brx    (Assembler::notZero, false, Assembler::pn, done) ;
         delayed()->nop() ;
         // m->owner == null : it's unlocked.
      }

      // Try to CAS m->owner from null to Self
      // Invariant: if we acquire the lock then _recursions should be 0.
      add    (Rmark, ObjectMonitor::owner_offset_in_bytes()-2, Rmark) ;
      mov    (G2_thread, Rscratch) ;
      casn   (Rmark, G0, Rscratch) ;
      cmp    (Rscratch, G0) ;
      // Intentional fall-through into done
   } else {
      // Aggressively avoid the Store-before-CAS penalty
      // Defer the store into box->dhw until after the CAS
      Label IsInflated, Recursive ;

// Anticipate CAS -- Avoid RTS->RTO upgrade
// prefetch (mark_addr, Assembler::severalWritesAndPossiblyReads) ;

      ld_ptr (mark_addr, Rmark);           // fetch obj->mark
      // Triage: biased, stack-locked, neutral, inflated

      if (UseBiasedLocking) {
        biased_locking_enter(Roop, Rmark, Rscratch, done, NULL, counters);
        // Invariant: if control reaches this point in the emitted stream
        // then Rmark has not been modified.
      }
      andcc  (Rmark, 2, G0) ;
      brx    (Assembler::notZero, false, Assembler::pn, IsInflated) ;
      delayed()->                         // Beware - dangling delay-slot

      // Try stack-lock acquisition.
      // Transiently install BUSY (0) encoding in the mark word.
      // if the CAS of 0 into the mark was successful then we execute:
      //   ST box->dhw  = mark   -- save fetched mark in on-stack basiclock box
      //   ST obj->mark = box    -- overwrite transient 0 value
      // This presumes TSO, of course.

      mov    (0, Rscratch) ;
      or3    (Rmark, markOopDesc::unlocked_value, Rmark);
      assert (mark_addr.disp() == 0, "cas must take a zero displacement");
      casn   (mark_addr.base(), Rmark, Rscratch) ;
// prefetch (mark_addr, Assembler::severalWritesAndPossiblyReads) ;
      cmp    (Rscratch, Rmark) ;
      brx    (Assembler::notZero, false, Assembler::pn, Recursive) ;
      delayed() ->
        st_ptr (Rmark, Rbox, BasicLock::displaced_header_offset_in_bytes());
      if (counters != NULL) {
        cond_inc(Assembler::equal, (address) counters->fast_path_entry_count_addr(), Rmark, Rscratch);
      }
      br     (Assembler::always, false, Assembler::pt, done);
      delayed() ->
        st_ptr (Rbox, mark_addr) ;

      bind   (Recursive) ;
      // Stack-lock attempt failed - check for recursive stack-lock.
      // Tests show that we can remove the recursive case with no impact
      // on refworkload 0.83.  If we need to reduce the size of the code
      // emitted by compiler_lock_object() the recursive case is perfect
      // candidate.
      //
      // A more extreme idea is to always inflate on stack-lock recursion.
      // This lets us eliminate the recursive checks in compiler_lock_object
      // and compiler_unlock_object and the (box->dhw == 0) encoding.
      // A brief experiment - requiring changes to synchronizer.cpp, interpreter,
      // and showed a performance *increase*.  In the same experiment I eliminated
      // the fast-path stack-lock code from the interpreter and always passed
      // control to the "slow" operators in synchronizer.cpp.

      // RScratch contains the fetched obj->mark value from the failed CASN.
#ifdef _LP64
      sub    (Rscratch, STACK_BIAS, Rscratch);
#endif
      sub(Rscratch, SP, Rscratch);
      assert(os::vm_page_size() > 0xfff, "page size too small - change the constant");
      andcc  (Rscratch, 0xfffff003, Rscratch);
      if (counters != NULL) {
        // Accounting needs the Rscratch register
        st_ptr (Rscratch, Rbox, BasicLock::displaced_header_offset_in_bytes());
        cond_inc(Assembler::equal, (address) counters->fast_path_entry_count_addr(), Rmark, Rscratch);
        br     (Assembler::always, false, Assembler::pt, done) ;
        delayed()->nop() ;
      } else {
        br     (Assembler::always, false, Assembler::pt, done) ;
        delayed()-> st_ptr (Rscratch, Rbox, BasicLock::displaced_header_offset_in_bytes());
      }

      bind   (IsInflated) ;
      if (EmitSync & 64) {
         // If m->owner != null goto IsLocked
         // Test-and-CAS vs CAS
         // Pessimistic form avoids futile (doomed) CAS attempts
         // The optimistic form avoids RTS->RTO cache line upgrades.
         ld_ptr (Address (Rmark, 0, ObjectMonitor::owner_offset_in_bytes()-2), Rscratch) ;
         andcc  (Rscratch, Rscratch, G0) ;
         brx    (Assembler::notZero, false, Assembler::pn, done) ;
         delayed()->nop() ;
         // m->owner == null : it's unlocked.
      }

      // Try to CAS m->owner from null to Self
      // Invariant: if we acquire the lock then _recursions should be 0.
      add    (Rmark, ObjectMonitor::owner_offset_in_bytes()-2, Rmark) ;
      mov    (G2_thread, Rscratch) ;
      casn   (Rmark, G0, Rscratch) ;
      cmp    (Rscratch, G0) ;
      // ST box->displaced_header = NonZero.
      // Any non-zero value suffices:
      //    unused_mark(), G2_thread, RBox, RScratch, rsp, etc.
      st_ptr (Rbox, Rbox, BasicLock::displaced_header_offset_in_bytes());
      // Intentional fall-through into done
   }

   bind   (done) ;
}

void MacroAssembler::compiler_unlock_object(Register Roop, Register Rmark, Register Rbox, Register Rscratch) {
   Address mark_addr(Roop, 0, oopDesc::mark_offset_in_bytes());

   Label done ;

   if (EmitSync & 4) {
     cmp  (SP, G0) ;
     return ;
   }

   if (EmitSync & 8) {
     if (UseBiasedLocking) {
        biased_locking_exit(mark_addr, Rscratch, done);
     }

     // Test first if it is a fast recursive unlock
     ld_ptr(Rbox, BasicLock::displaced_header_offset_in_bytes(), Rmark);
     cmp(Rmark, G0);
     brx(Assembler::equal, false, Assembler::pt, done);
     delayed()->nop();

     // Check if it is still a light weight lock, this is is true if we see
     // the stack address of the basicLock in the markOop of the object
     assert(mark_addr.disp() == 0, "cas must take a zero displacement");
     casx_under_lock(mark_addr.base(), Rbox, Rmark,
       (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr());
     br (Assembler::always, false, Assembler::pt, done);
     delayed()->cmp(Rbox, Rmark);
     bind (done) ;
     return ;
   }

   // Beware ... If the aggregate size of the code emitted by CLO and CUO is
   // is too large performance rolls abruptly off a cliff.
   // This could be related to inlining policies, code cache management, or
   // I$ effects.
   Label LStacked ;

   if (UseBiasedLocking) {
      // TODO: eliminate redundant LDs of obj->mark
      biased_locking_exit(mark_addr, Rscratch, done);
   }

   ld_ptr (Roop, oopDesc::mark_offset_in_bytes(), Rmark) ;
   ld_ptr (Rbox, BasicLock::displaced_header_offset_in_bytes(), Rscratch);
   andcc  (Rscratch, Rscratch, G0);
   brx    (Assembler::zero, false, Assembler::pn, done);
   delayed()-> nop() ;      // consider: relocate fetch of mark, above, into this DS
   andcc  (Rmark, 2, G0) ;
   brx    (Assembler::zero, false, Assembler::pt, LStacked) ;
   delayed()-> nop() ;

   // It's inflated
   // Conceptually we need a #loadstore|#storestore "release" MEMBAR before
   // the ST of 0 into _owner which releases the lock.  This prevents loads
   // and stores within the critical section from reordering (floating)
   // past the store that releases the lock.  But TSO is a strong memory model
   // and that particular flavor of barrier is a noop, so we can safely elide it.
   // Note that we use 1-0 locking by default for the inflated case.  We
   // close the resultant (and rare) race by having contented threads in
   // monitorenter periodically poll _owner.
   ld_ptr (Address(Rmark, 0, ObjectMonitor::owner_offset_in_bytes()-2), Rscratch) ;
   ld_ptr (Address(Rmark, 0, ObjectMonitor::recursions_offset_in_bytes()-2), Rbox) ;
   xor3   (Rscratch, G2_thread, Rscratch) ;
   orcc   (Rbox, Rscratch, Rbox) ;
   brx    (Assembler::notZero, false, Assembler::pn, done) ;
   delayed()->
   ld_ptr (Address (Rmark, 0, ObjectMonitor::EntryList_offset_in_bytes()-2), Rscratch) ;
   ld_ptr (Address (Rmark, 0, ObjectMonitor::cxq_offset_in_bytes()-2), Rbox) ;
   orcc   (Rbox, Rscratch, G0) ;
   if (EmitSync & 65536) {
      Label LSucc ;
      brx    (Assembler::notZero, false, Assembler::pn, LSucc) ;
      delayed()->nop() ;
      br     (Assembler::always, false, Assembler::pt, done) ;
      delayed()->
      st_ptr (G0, Address (Rmark, 0, ObjectMonitor::owner_offset_in_bytes()-2)) ;

      bind   (LSucc) ;
      st_ptr (G0, Address (Rmark, 0, ObjectMonitor::owner_offset_in_bytes()-2)) ;
      if (os::is_MP()) { membar (StoreLoad) ; }
      ld_ptr (Address (Rmark, 0, ObjectMonitor::succ_offset_in_bytes()-2), Rscratch) ;
      andcc  (Rscratch, Rscratch, G0) ;
      brx    (Assembler::notZero, false, Assembler::pt, done) ;
      delayed()-> andcc (G0, G0, G0) ;
      add    (Rmark, ObjectMonitor::owner_offset_in_bytes()-2, Rmark) ;
      mov    (G2_thread, Rscratch) ;
      casn   (Rmark, G0, Rscratch) ;
      cmp    (Rscratch, G0) ;
      // invert icc.zf and goto done
      brx    (Assembler::notZero, false, Assembler::pt, done) ;
      delayed() -> cmp (G0, G0) ;
      br     (Assembler::always, false, Assembler::pt, done);
      delayed() -> cmp (G0, 1) ;
   } else {
      brx    (Assembler::notZero, false, Assembler::pn, done) ;
      delayed()->nop() ;
      br     (Assembler::always, false, Assembler::pt, done) ;
      delayed()->
      st_ptr (G0, Address (Rmark, 0, ObjectMonitor::owner_offset_in_bytes()-2)) ;
   }

   bind   (LStacked) ;
   // Consider: we could replace the expensive CAS in the exit
   // path with a simple ST of the displaced mark value fetched from
   // the on-stack basiclock box.  That admits a race where a thread T2
   // in the slow lock path -- inflating with monitor M -- could race a
   // thread T1 in the fast unlock path, resulting in a missed wakeup for T2.
   // More precisely T1 in the stack-lock unlock path could "stomp" the
   // inflated mark value M installed by T2, resulting in an orphan
   // object monitor M and T2 becoming stranded.  We can remedy that situation
   // by having T2 periodically poll the object's mark word using timed wait
   // operations.  If T2 discovers that a stomp has occurred it vacates
   // the monitor M and wakes any other threads stranded on the now-orphan M.
   // In addition the monitor scavenger, which performs deflation,
   // would also need to check for orpan monitors and stranded threads.
   //
   // Finally, inflation is also used when T2 needs to assign a hashCode
   // to O and O is stack-locked by T1.  The "stomp" race could cause
   // an assigned hashCode value to be lost.  We can avoid that condition
   // and provide the necessary hashCode stability invariants by ensuring
   // that hashCode generation is idempotent between copying GCs.
   // For example we could compute the hashCode of an object O as
   // O's heap address XOR some high quality RNG value that is refreshed
   // at GC-time.  The monitor scavenger would install the hashCode
   // found in any orphan monitors.  Again, the mechanism admits a
   // lost-update "stomp" WAW race but detects and recovers as needed.
   //
   // A prototype implementation showed excellent results, although
   // the scavenger and timeout code was rather involved.

   casn   (mark_addr.base(), Rbox, Rscratch) ;
   cmp    (Rbox, Rscratch);
   // Intentional fall through into done ...

   bind   (done) ;
}



void MacroAssembler::print_CPU_state() {
  // %%%%% need to implement this
}

void MacroAssembler::verify_FPU(int stack_depth, const char* s) {
  // %%%%% need to implement this
}

void MacroAssembler::push_IU_state() {
  // %%%%% need to implement this
}


void MacroAssembler::pop_IU_state() {
  // %%%%% need to implement this
}


void MacroAssembler::push_FPU_state() {
  // %%%%% need to implement this
}


void MacroAssembler::pop_FPU_state() {
  // %%%%% need to implement this
}


void MacroAssembler::push_CPU_state() {
  // %%%%% need to implement this
}


void MacroAssembler::pop_CPU_state() {
  // %%%%% need to implement this
}



void MacroAssembler::verify_tlab() {
#ifdef ASSERT
  if (UseTLAB && VerifyOops) {
    Label next, next2, ok;
    Register t1 = L0;
    Register t2 = L1;
    Register t3 = L2;

    save_frame(0);
    ld_ptr(G2_thread, in_bytes(JavaThread::tlab_top_offset()), t1);
    ld_ptr(G2_thread, in_bytes(JavaThread::tlab_start_offset()), t2);
    or3(t1, t2, t3);
    cmp(t1, t2);
    br(Assembler::greaterEqual, false, Assembler::pn, next);
    delayed()->nop();
    stop("assert(top >= start)");
    should_not_reach_here();

    bind(next);
    ld_ptr(G2_thread, in_bytes(JavaThread::tlab_top_offset()), t1);
    ld_ptr(G2_thread, in_bytes(JavaThread::tlab_end_offset()), t2);
    or3(t3, t2, t3);
    cmp(t1, t2);
    br(Assembler::lessEqual, false, Assembler::pn, next2);
    delayed()->nop();
    stop("assert(top <= end)");
    should_not_reach_here();

    bind(next2);
    and3(t3, MinObjAlignmentInBytesMask, t3);
    cmp(t3, 0);
    br(Assembler::lessEqual, false, Assembler::pn, ok);
    delayed()->nop();
    stop("assert(aligned)");
    should_not_reach_here();

    bind(ok);
    restore();
  }
#endif
}


void MacroAssembler::eden_allocate(
  Register obj,                        // result: pointer to object after successful allocation
  Register var_size_in_bytes,          // object size in bytes if unknown at compile time; invalid otherwise
  int      con_size_in_bytes,          // object size in bytes if   known at compile time
  Register t1,                         // temp register
  Register t2,                         // temp register
  Label&   slow_case                   // continuation point if fast allocation fails
){
  // make sure arguments make sense
  assert_different_registers(obj, var_size_in_bytes, t1, t2);
  assert(0 <= con_size_in_bytes && Assembler::is_simm13(con_size_in_bytes), "illegal object size");
  assert((con_size_in_bytes & MinObjAlignmentInBytesMask) == 0, "object size is not multiple of alignment");

  // get eden boundaries
  // note: we need both top & top_addr!
  const Register top_addr = t1;
  const Register end      = t2;

  CollectedHeap* ch = Universe::heap();
  set((intx)ch->top_addr(), top_addr);
  intx delta = (intx)ch->end_addr() - (intx)ch->top_addr();
  ld_ptr(top_addr, delta, end);
  ld_ptr(top_addr, 0, obj);

  // try to allocate
  Label retry;
  bind(retry);
#ifdef ASSERT
  // make sure eden top is properly aligned
  {
    Label L;
    btst(MinObjAlignmentInBytesMask, obj);
    br(Assembler::zero, false, Assembler::pt, L);
    delayed()->nop();
    stop("eden top is not properly aligned");
    bind(L);
  }
#endif // ASSERT
  const Register free = end;
  sub(end, obj, free);                                   // compute amount of free space
  if (var_size_in_bytes->is_valid()) {
    // size is unknown at compile time
    cmp(free, var_size_in_bytes);
    br(Assembler::lessUnsigned, false, Assembler::pn, slow_case); // if there is not enough space go the slow case
    delayed()->add(obj, var_size_in_bytes, end);
  } else {
    // size is known at compile time
    cmp(free, con_size_in_bytes);
    br(Assembler::lessUnsigned, false, Assembler::pn, slow_case); // if there is not enough space go the slow case
    delayed()->add(obj, con_size_in_bytes, end);
  }
  // Compare obj with the value at top_addr; if still equal, swap the value of
  // end with the value at top_addr. If not equal, read the value at top_addr
  // into end.
  casx_under_lock(top_addr, obj, end, (address)StubRoutines::Sparc::atomic_memory_operation_lock_addr());
  // if someone beat us on the allocation, try again, otherwise continue
  cmp(obj, end);
  brx(Assembler::notEqual, false, Assembler::pn, retry);
  delayed()->mov(end, obj);                              // nop if successfull since obj == end

#ifdef ASSERT
  // make sure eden top is properly aligned
  {
    Label L;
    const Register top_addr = t1;

    set((intx)ch->top_addr(), top_addr);
    ld_ptr(top_addr, 0, top_addr);
    btst(MinObjAlignmentInBytesMask, top_addr);
    br(Assembler::zero, false, Assembler::pt, L);
    delayed()->nop();
    stop("eden top is not properly aligned");
    bind(L);
  }
#endif // ASSERT
}


void MacroAssembler::tlab_allocate(
  Register obj,                        // result: pointer to object after successful allocation
  Register var_size_in_bytes,          // object size in bytes if unknown at compile time; invalid otherwise
  int      con_size_in_bytes,          // object size in bytes if   known at compile time
  Register t1,                         // temp register
  Label&   slow_case                   // continuation point if fast allocation fails
){
  // make sure arguments make sense
  assert_different_registers(obj, var_size_in_bytes, t1);
  assert(0 <= con_size_in_bytes && is_simm13(con_size_in_bytes), "illegal object size");
  assert((con_size_in_bytes & MinObjAlignmentInBytesMask) == 0, "object size is not multiple of alignment");

  const Register free  = t1;

  verify_tlab();

  ld_ptr(G2_thread, in_bytes(JavaThread::tlab_top_offset()), obj);

  // calculate amount of free space
  ld_ptr(G2_thread, in_bytes(JavaThread::tlab_end_offset()), free);
  sub(free, obj, free);

  Label done;
  if (var_size_in_bytes == noreg) {
    cmp(free, con_size_in_bytes);
  } else {
    cmp(free, var_size_in_bytes);
  }
  br(Assembler::less, false, Assembler::pn, slow_case);
  // calculate the new top pointer
  if (var_size_in_bytes == noreg) {
    delayed()->add(obj, con_size_in_bytes, free);
  } else {
    delayed()->add(obj, var_size_in_bytes, free);
  }

  bind(done);

#ifdef ASSERT
  // make sure new free pointer is properly aligned
  {
    Label L;
    btst(MinObjAlignmentInBytesMask, free);
    br(Assembler::zero, false, Assembler::pt, L);
    delayed()->nop();
    stop("updated TLAB free is not properly aligned");
    bind(L);
  }
#endif // ASSERT

  // update the tlab top pointer
  st_ptr(free, G2_thread, in_bytes(JavaThread::tlab_top_offset()));
  verify_tlab();
}


void MacroAssembler::tlab_refill(Label& retry, Label& try_eden, Label& slow_case) {
  Register top = O0;
  Register t1 = G1;
  Register t2 = G3;
  Register t3 = O1;
  assert_different_registers(top, t1, t2, t3, G4, G5 /* preserve G4 and G5 */);
  Label do_refill, discard_tlab;

  if (CMSIncrementalMode || !Universe::heap()->supports_inline_contig_alloc()) {
    // No allocation in the shared eden.
    br(Assembler::always, false, Assembler::pt, slow_case);
    delayed()->nop();
  }

  ld_ptr(G2_thread, in_bytes(JavaThread::tlab_top_offset()), top);
  ld_ptr(G2_thread, in_bytes(JavaThread::tlab_end_offset()), t1);
  ld_ptr(G2_thread, in_bytes(JavaThread::tlab_refill_waste_limit_offset()), t2);

  // calculate amount of free space
  sub(t1, top, t1);
  srl_ptr(t1, LogHeapWordSize, t1);

  // Retain tlab and allocate object in shared space if
  // the amount free in the tlab is too large to discard.
  cmp(t1, t2);
  brx(Assembler::lessEqual, false, Assembler::pt, discard_tlab);

  // increment waste limit to prevent getting stuck on this slow path
  delayed()->add(t2, ThreadLocalAllocBuffer::refill_waste_limit_increment(), t2);
  st_ptr(t2, G2_thread, in_bytes(JavaThread::tlab_refill_waste_limit_offset()));
  if (TLABStats) {
    // increment number of slow_allocations
    ld(G2_thread, in_bytes(JavaThread::tlab_slow_allocations_offset()), t2);
    add(t2, 1, t2);
    stw(t2, G2_thread, in_bytes(JavaThread::tlab_slow_allocations_offset()));
  }
  br(Assembler::always, false, Assembler::pt, try_eden);
  delayed()->nop();

  bind(discard_tlab);
  if (TLABStats) {
    // increment number of refills
    ld(G2_thread, in_bytes(JavaThread::tlab_number_of_refills_offset()), t2);
    add(t2, 1, t2);
    stw(t2, G2_thread, in_bytes(JavaThread::tlab_number_of_refills_offset()));
    // accumulate wastage
    ld(G2_thread, in_bytes(JavaThread::tlab_fast_refill_waste_offset()), t2);
    add(t2, t1, t2);
    stw(t2, G2_thread, in_bytes(JavaThread::tlab_fast_refill_waste_offset()));
  }

  // if tlab is currently allocated (top or end != null) then
  // fill [top, end + alignment_reserve) with array object
  br_null(top, false, Assembler::pn, do_refill);
  delayed()->nop();

  set((intptr_t)markOopDesc::prototype()->copy_set_hash(0x2), t2);
  st_ptr(t2, top, oopDesc::mark_offset_in_bytes()); // set up the mark word
  // set klass to intArrayKlass
  sub(t1, typeArrayOopDesc::header_size(T_INT), t1);
  add(t1, ThreadLocalAllocBuffer::alignment_reserve(), t1);
  sll_ptr(t1, log2_intptr(HeapWordSize/sizeof(jint)), t1);
  st(t1, top, arrayOopDesc::length_offset_in_bytes());
  set((intptr_t)Universe::intArrayKlassObj_addr(), t2);
  ld_ptr(t2, 0, t2);
  // store klass last.  concurrent gcs assumes klass length is valid if
  // klass field is not null.
  store_klass(t2, top);
  verify_oop(top);

  // refill the tlab with an eden allocation
  bind(do_refill);
  ld_ptr(G2_thread, in_bytes(JavaThread::tlab_size_offset()), t1);
  sll_ptr(t1, LogHeapWordSize, t1);
  // add object_size ??
  eden_allocate(top, t1, 0, t2, t3, slow_case);

  st_ptr(top, G2_thread, in_bytes(JavaThread::tlab_start_offset()));
  st_ptr(top, G2_thread, in_bytes(JavaThread::tlab_top_offset()));
#ifdef ASSERT
  // check that tlab_size (t1) is still valid
  {
    Label ok;
    ld_ptr(G2_thread, in_bytes(JavaThread::tlab_size_offset()), t2);
    sll_ptr(t2, LogHeapWordSize, t2);
    cmp(t1, t2);
    br(Assembler::equal, false, Assembler::pt, ok);
    delayed()->nop();
    stop("assert(t1 == tlab_size)");
    should_not_reach_here();

    bind(ok);
  }
#endif // ASSERT
  add(top, t1, top); // t1 is tlab_size
  sub(top, ThreadLocalAllocBuffer::alignment_reserve_in_bytes(), top);
  st_ptr(top, G2_thread, in_bytes(JavaThread::tlab_end_offset()));
  verify_tlab();
  br(Assembler::always, false, Assembler::pt, retry);
  delayed()->nop();
}

Assembler::Condition MacroAssembler::negate_condition(Assembler::Condition cond) {
  switch (cond) {
    // Note some conditions are synonyms for others
    case Assembler::never:                return Assembler::always;
    case Assembler::zero:                 return Assembler::notZero;
    case Assembler::lessEqual:            return Assembler::greater;
    case Assembler::less:                 return Assembler::greaterEqual;
    case Assembler::lessEqualUnsigned:    return Assembler::greaterUnsigned;
    case Assembler::lessUnsigned:         return Assembler::greaterEqualUnsigned;
    case Assembler::negative:             return Assembler::positive;
    case Assembler::overflowSet:          return Assembler::overflowClear;
    case Assembler::always:               return Assembler::never;
    case Assembler::notZero:              return Assembler::zero;
    case Assembler::greater:              return Assembler::lessEqual;
    case Assembler::greaterEqual:         return Assembler::less;
    case Assembler::greaterUnsigned:      return Assembler::lessEqualUnsigned;
    case Assembler::greaterEqualUnsigned: return Assembler::lessUnsigned;
    case Assembler::positive:             return Assembler::negative;
    case Assembler::overflowClear:        return Assembler::overflowSet;
  }

  ShouldNotReachHere(); return Assembler::overflowClear;
}

void MacroAssembler::cond_inc(Assembler::Condition cond, address counter_ptr,
                              Register Rtmp1, Register Rtmp2 /*, Register Rtmp3, Register Rtmp4 */) {
  Condition negated_cond = negate_condition(cond);
  Label L;
  brx(negated_cond, false, Assembler::pt, L);
  delayed()->nop();
  inc_counter(counter_ptr, Rtmp1, Rtmp2);
  bind(L);
}

void MacroAssembler::inc_counter(address counter_ptr, Register Rtmp1, Register Rtmp2) {
  Address counter_addr(Rtmp1, counter_ptr);
  load_contents(counter_addr, Rtmp2);
  inc(Rtmp2);
  store_contents(Rtmp2, counter_addr);
}

SkipIfEqual::SkipIfEqual(
    MacroAssembler* masm, Register temp, const bool* flag_addr,
    Assembler::Condition condition) {
  _masm = masm;
  Address flag(temp, (address)flag_addr, relocInfo::none);
  _masm->sethi(flag);
  _masm->ldub(flag, temp);
  _masm->tst(temp);
  _masm->br(condition, false, Assembler::pt, _label);
  _masm->delayed()->nop();
}

SkipIfEqual::~SkipIfEqual() {
  _masm->bind(_label);
}


// Writes to stack successive pages until offset reached to check for
// stack overflow + shadow pages.  This clobbers tsp and scratch.
void MacroAssembler::bang_stack_size(Register Rsize, Register Rtsp,
                                     Register Rscratch) {
  // Use stack pointer in temp stack pointer
  mov(SP, Rtsp);

  // Bang stack for total size given plus stack shadow page size.
  // Bang one page at a time because a large size can overflow yellow and
  // red zones (the bang will fail but stack overflow handling can't tell that
  // it was a stack overflow bang vs a regular segv).
  int offset = os::vm_page_size();
  Register Roffset = Rscratch;

  Label loop;
  bind(loop);
  set((-offset)+STACK_BIAS, Rscratch);
  st(G0, Rtsp, Rscratch);
  set(offset, Roffset);
  sub(Rsize, Roffset, Rsize);
  cmp(Rsize, G0);
  br(Assembler::greater, false, Assembler::pn, loop);
  delayed()->sub(Rtsp, Roffset, Rtsp);

  // Bang down shadow pages too.
  // The -1 because we already subtracted 1 page.
  for (int i = 0; i< StackShadowPages-1; i++) {
    set((-i*offset)+STACK_BIAS, Rscratch);
    st(G0, Rtsp, Rscratch);
  }
}

void MacroAssembler::load_klass(Register src_oop, Register klass) {
  // The number of bytes in this code is used by
  // MachCallDynamicJavaNode::ret_addr_offset()
  // if this changes, change that.
  if (UseCompressedOops) {
    lduw(src_oop, oopDesc::klass_offset_in_bytes(), klass);
    decode_heap_oop_not_null(klass);
  } else {
    ld_ptr(src_oop, oopDesc::klass_offset_in_bytes(), klass);
  }
}

void MacroAssembler::store_klass(Register klass, Register dst_oop) {
  if (UseCompressedOops) {
    assert(dst_oop != klass, "not enough registers");
    encode_heap_oop_not_null(klass);
    st(klass, dst_oop, oopDesc::klass_offset_in_bytes());
  } else {
    st_ptr(klass, dst_oop, oopDesc::klass_offset_in_bytes());
  }
}

void MacroAssembler::store_klass_gap(Register s, Register d) {
  if (UseCompressedOops) {
    assert(s != d, "not enough registers");
    st(s, d, oopDesc::klass_gap_offset_in_bytes());
  }
}

void MacroAssembler::load_heap_oop(const Address& s, Register d, int offset) {
  if (UseCompressedOops) {
    lduw(s, d, offset);
    decode_heap_oop(d);
  } else {
    ld_ptr(s, d, offset);
  }
}

void MacroAssembler::load_heap_oop(Register s1, Register s2, Register d) {
   if (UseCompressedOops) {
    lduw(s1, s2, d);
    decode_heap_oop(d, d);
  } else {
    ld_ptr(s1, s2, d);
  }
}

void MacroAssembler::load_heap_oop(Register s1, int simm13a, Register d) {
   if (UseCompressedOops) {
    lduw(s1, simm13a, d);
    decode_heap_oop(d, d);
  } else {
    ld_ptr(s1, simm13a, d);
  }
}

void MacroAssembler::store_heap_oop(Register d, Register s1, Register s2) {
  if (UseCompressedOops) {
    assert(s1 != d && s2 != d, "not enough registers");
    encode_heap_oop(d);
    st(d, s1, s2);
  } else {
    st_ptr(d, s1, s2);
  }
}

void MacroAssembler::store_heap_oop(Register d, Register s1, int simm13a) {
  if (UseCompressedOops) {
    assert(s1 != d, "not enough registers");
    encode_heap_oop(d);
    st(d, s1, simm13a);
  } else {
    st_ptr(d, s1, simm13a);
  }
}

void MacroAssembler::store_heap_oop(Register d, const Address& a, int offset) {
  if (UseCompressedOops) {
    assert(a.base() != d, "not enough registers");
    encode_heap_oop(d);
    st(d, a, offset);
  } else {
    st_ptr(d, a, offset);
  }
}


void MacroAssembler::encode_heap_oop(Register src, Register dst) {
  assert (UseCompressedOops, "must be compressed");
  verify_oop(src);
  Label done;
  if (src == dst) {
    // optimize for frequent case src == dst
    bpr(rc_nz, true, Assembler::pt, src, done);
    delayed() -> sub(src, G6_heapbase, dst); // annuled if not taken
    bind(done);
    srlx(src, LogMinObjAlignmentInBytes, dst);
  } else {
    bpr(rc_z, false, Assembler::pn, src, done);
    delayed() -> mov(G0, dst);
    // could be moved before branch, and annulate delay,
    // but may add some unneeded work decoding null
    sub(src, G6_heapbase, dst);
    srlx(dst, LogMinObjAlignmentInBytes, dst);
    bind(done);
  }
}


void MacroAssembler::encode_heap_oop_not_null(Register r) {
  assert (UseCompressedOops, "must be compressed");
  verify_oop(r);
  sub(r, G6_heapbase, r);
  srlx(r, LogMinObjAlignmentInBytes, r);
}

void MacroAssembler::encode_heap_oop_not_null(Register src, Register dst) {
  assert (UseCompressedOops, "must be compressed");
  verify_oop(src);
  sub(src, G6_heapbase, dst);
  srlx(dst, LogMinObjAlignmentInBytes, dst);
}

// Same algorithm as oops.inline.hpp decode_heap_oop.
void  MacroAssembler::decode_heap_oop(Register src, Register dst) {
  assert (UseCompressedOops, "must be compressed");
  Label done;
  sllx(src, LogMinObjAlignmentInBytes, dst);
  bpr(rc_nz, true, Assembler::pt, dst, done);
  delayed() -> add(dst, G6_heapbase, dst); // annuled if not taken
  bind(done);
  verify_oop(dst);
}

void  MacroAssembler::decode_heap_oop_not_null(Register r) {
  // Do not add assert code to this unless you change vtableStubs_sparc.cpp
  // pd_code_size_limit.
  // Also do not verify_oop as this is called by verify_oop.
  assert (UseCompressedOops, "must be compressed");
  sllx(r, LogMinObjAlignmentInBytes, r);
  add(r, G6_heapbase, r);
}

void  MacroAssembler::decode_heap_oop_not_null(Register src, Register dst) {
  // Do not add assert code to this unless you change vtableStubs_sparc.cpp
  // pd_code_size_limit.
  // Also do not verify_oop as this is called by verify_oop.
  assert (UseCompressedOops, "must be compressed");
  sllx(src, LogMinObjAlignmentInBytes, dst);
  add(dst, G6_heapbase, dst);
}

void MacroAssembler::reinit_heapbase() {
  if (UseCompressedOops) {
    // call indirectly to solve generation ordering problem
    Address base(G6_heapbase, (address)Universe::heap_base_addr());
    load_ptr_contents(base, G6_heapbase);
  }
}
