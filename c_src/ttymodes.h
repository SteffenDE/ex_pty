// adapted from:
// https://github.com/openssh/openssh-portable/blob/2dc328023f60212cd29504fc05d849133ae47355/ttymodes.h
/* $OpenBSD: ttymodes.h,v 1.16 2017/04/30 23:26:54 djm Exp $ */

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

/*
 * SSH2 tty modes support by Kevin Steves.
 * Copyright (c) 2001 Kevin Steves.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The tty mode description is a string, consisting of
 * opcode-arguments pairs.  It is terminated by opcode TTY_OP_END (0).
 * Opcodes 1-159 have uint32 arguments.
 * Opcodes 160-255 are not yet defined and cause parsing to stop (they
 * should only be used after any other data).
 *
 * The client puts in the string any modes it knows about, and the
 * server ignores any modes it does not know about.  This allows some degree
 * of machine-independence, at least between systems that use a posix-like
 * tty interface.  The protocol can support other systems as well, but might
 * require reimplementing as mode names would likely be different.
 */

/*
 * Some constants and prototypes are defined in packet.h; this file
 * is only intended for including from ttymodes.c.
 */

#define TTY_OP_ISPEED 128
#define TTY_OP_OSPEED 129

/* termios macro */
/* name, op */
TTYCHAR(VINTR, "vintr")
TTYCHAR(VQUIT, "vquit")
TTYCHAR(VERASE, "verase")
#if defined(VKILL)
TTYCHAR(VKILL, "vkill")
#endif /* VKILL */
TTYCHAR(VEOF, "veof")
#if defined(VEOL)
TTYCHAR(VEOL, "veol")
#endif /* VEOL */
#ifdef VEOL2
TTYCHAR(VEOL2, "veol2")
#endif /* VEOL2 */
TTYCHAR(VSTART, "vstart")
TTYCHAR(VSTOP, "vstop")
#if defined(VSUSP)
TTYCHAR(VSUSP, "vsusp")
#endif /* VSUSP */
#if defined(VDSUSP)
TTYCHAR(VDSUSP, "vdsusp")
#endif /* VDSUSP */
#if defined(VREPRINT)
TTYCHAR(VREPRINT, "vreprint")
#endif /* VREPRINT */
#if defined(VWERASE)
TTYCHAR(VWERASE, "vwerase")
#endif /* VWERASE */
#if defined(VLNEXT)
TTYCHAR(VLNEXT, "vlnext")
#endif /* VLNEXT */
#if defined(VFLUSH)
TTYCHAR(VFLUSH, "vflush")
#endif /* VFLUSH */
#ifdef VSWTCH
TTYCHAR(VSWTCH, "vswtch")
#endif /* VSWTCH */
#if defined(VSTATUS)
TTYCHAR(VSTATUS, "vstatus")
#endif /* VSTATUS */
#ifdef VDISCARD
TTYCHAR(VDISCARD, "vdiscard")
#endif /* VDISCARD */

/* name, field, op */
TTYMODE(IGNPAR, c_iflag, "ignpar")
TTYMODE(PARMRK, c_iflag, "parmrk")
TTYMODE(INPCK, c_iflag, "inpck")
TTYMODE(ISTRIP, c_iflag, "istrip")
TTYMODE(INLCR, c_iflag, "inlcr")
TTYMODE(IGNCR, c_iflag, "igncr")
TTYMODE(ICRNL, c_iflag, "icrnl")
#if defined(IUCLC)
TTYMODE(IUCLC, c_iflag, "iuclc")
#endif
TTYMODE(IXON, c_iflag, "ixon")
TTYMODE(IXANY, c_iflag, "ixany")
TTYMODE(IXOFF, c_iflag, "ixoff")
#ifdef IMAXBEL
TTYMODE(IMAXBEL, c_iflag, "imaxbel")
#endif /* IMAXBEL */
#ifdef IUTF8
TTYMODE(IUTF8, c_iflag, "iutf8")
#endif /* IUTF8 */

TTYMODE(ISIG, c_lflag, "isig")
TTYMODE(ICANON, c_lflag, "icanon")
#ifdef XCASE
TTYMODE(XCASE, c_lflag, "xcase")
#endif
TTYMODE(ECHO, c_lflag, "echo")
TTYMODE(ECHOE, c_lflag, "echoe")
TTYMODE(ECHOK, c_lflag, "echok")
TTYMODE(ECHONL, c_lflag, "echonl")
TTYMODE(NOFLSH, c_lflag, "noflsh")
TTYMODE(TOSTOP, c_lflag, "tostop")
#ifdef IEXTEN
TTYMODE(IEXTEN, c_lflag, "iexten")
#endif /* IEXTEN */
#if defined(ECHOCTL)
TTYMODE(ECHOCTL, c_lflag, "echoctl")
#endif /* ECHOCTL */
#ifdef ECHOKE
TTYMODE(ECHOKE, c_lflag, "echoke")
#endif /* ECHOKE */
#if defined(PENDIN)
TTYMODE(PENDIN, c_lflag, "pendin")
#endif /* PENDIN */

TTYMODE(OPOST, c_oflag, "opost")
#if defined(OLCUC)
TTYMODE(OLCUC, c_oflag, "olcuc")
#endif
#ifdef ONLCR
TTYMODE(ONLCR, c_oflag, "onlcr")
#endif
#ifdef OCRNL
TTYMODE(OCRNL, c_oflag, "ocrnl")
#endif
#ifdef ONOCR
TTYMODE(ONOCR, c_oflag, "onocr")
#endif
#ifdef ONLRET
TTYMODE(ONLRET, c_oflag, "onlret")
#endif

TTYMODE(CS7, c_cflag, "cs7")
TTYMODE(CS8, c_cflag, "cs8")
TTYMODE(PARENB, c_cflag, "parenb")
TTYMODE(PARODD, c_cflag, "parodd")

TTYSPEED(TTY_OP_ISPEED, c_ispeed, "tty_op_ispeed")
TTYSPEED(TTY_OP_OSPEED, c_ospeed, "tty_op_ospeed")
