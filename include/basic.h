/**
 * @file basic.h
 * @brief Retro Rocket BASIC interpreter
 * 
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * @note loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
 * 
 * uBASIC is far more limited than the dialect implemented here. It only allowed
 * variables of one letter in length, and only integer variables, no PROC, FN,
 * or additional functions, no floating point or string ops, no INPUT,
 * just plain mathematical expressions, no ability to isolate execution into a
 * context, and was (and in parts still is) quite badly optimised. It was what
 * it was, a good starting off point.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#pragma once

#include "buddy_allocator.h"
#include "basic/defines.h"
#include "basic/structs.h"
#include "basic/context.h"
#include "basic/expression.h"
#include "basic/validation.h"
#include "basic/variable.h"

#include "basic/builtin_integer_functions.h"
#include "basic/builtin_string_functions.h"
#include "basic/builtin_file_io_functions.h"
#include "basic/builtin_real_functions.h"
#include "basic/flow_control.h"
#include "basic/lowlevel.h"
#include "basic/array.h"
#include "basic/sockets.h"
#include "basic/graphics.h"
#include "basic/console.h"
#include "basic/process.h"
#include "basic/reflection.h"
#include "basic/datetime.h"
