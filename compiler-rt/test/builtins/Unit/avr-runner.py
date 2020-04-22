#!/usr/bin/python3

import sys
import os
import tempfile
import subprocess

MCU = 'atmega1284p'
COMPILER = 'clang'
LIBC = True


def run_tests(tests, tmpdir):
    successful = 0
    failed = 0
    skipped = 0
    for test in tests:
        returncode, stderr = run_test(test, tmpdir)
        if returncode != 0:
            # test failed to compile/run
            failed += 1
            print('== fail:', test)
            if len(stderr) == 0:
                stderr = '<no output>'
            sys.stdout.write('\t' + stderr.rstrip().replace('\n', '\n\t') + '\n')
            continue

        # filter escape characters in the output
        stderr = filter_escape_sequences(stderr).replace('.\n', '\n')
        if stderr == 'ok\n':
            # test was successful
            successful += 1
            print('== ok:  ', test)
            continue
        if stderr == 'skipped\nok\n' or stderr == 'sok\n':
            # skipped this test
            skipped += 1
            print('== skip:', test)
            continue
        failed += 1

        # test has incorrect result
        print('== fail:', test)
        if len(stderr) == 0:
            stderr = '<no output>'
        sys.stdout.write('\t' + stderr.rstrip().replace('\n', '\n\t') + '\n')

    print('successful: %d\nskipped:    %d\nfailed:     %d' % (successful, skipped, failed))

def run_test(test, tmpdir):
    name = os.path.basename(test)
    objfile = os.path.join(tmpdir, name+'.o')
    executable = os.path.join(tmpdir, name+'.elf')

    # compile the program
    if COMPILER == 'clang':
        command = ['../../../../../llvm-build.master/bin/clang',
                   '--target=avr-unknown-unknown',
                   '-nostdlibinc',
                   '-mdouble=64',
                   '-I/usr/lib/avr/include']
    elif COMPILER == 'avr-gcc':
        command = ['avr-gcc']
    else:
        raise ValueError('unknown compiler: ' + COMPILER)

    command.extend([
            '-I../../../lib/builtins',         # for headers such as int_lib.h
            '-mmcu='+MCU,
            '-Os', '-std=c11',                 # 'default' optimization level
            '-gdwarf-4',
            '-c', '-o', objfile,
            '-DTESTCASE="'+test+'"',
            'avr-runner.c'])
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        return result.returncode, result.stdout

    # link the program
    linkcmd = [
        'avr-gcc',
        objfile,
        '-mmcu='+MCU,
        '/home/ayke/.cache/tinygo/compiler-rt-avr-atmel-none.a',
        '-Wl,--gc-sections',
        '-o', executable]
    if LIBC:
        linkcmd.extend([
            '-Wl,-u,vfprintf', '-lprintf_flt']) # use a more fully-featured printf function
    else:
        linkcmd.extend([
            '-nostdlib',
            '/usr/lib/avr/lib/avr51/crtatmega1284p.o'])
    result = subprocess.run(linkcmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        return result.returncode, result.stdout

    # run the test
    try:
        result = subprocess.run([
            'simavr',
            '-m', MCU,
            executable],
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=10)
        return result.returncode, result.stderr.decode()
    except subprocess.TimeoutExpired:
        return 1, '<timeout>'

def filter_escape_sequences(s):
    while 1:
        pos = s.find('\x1b')
        if pos < 0:
            # no escape characters left
            return s
        end = s.find('m', pos)
        if end < 0:
            # non-terminted escape sequence
            return s
        s = s[:pos] + s[end+1:]


default_tests = [
    'absvdi2_test.c',
    'absvsi2_test.c',
    'absvti2_test.c',
    #'adddf3vfp_test.c',
    #'addsf3vfp_test.c',
    'addtf3_test.c',
    'addvdi3_test.c',
    'addvsi3_test.c',
    'addvti3_test.c',
    'ashldi3_test.c',
    'ashlsi3_test.c',
    'ashlti3_test.c',
    'ashrdi3_test.c',
    'ashrsi3_test.c',
    'ashrti3_test.c',
    'bswapdi2_test.c',
    'bswapsi2_test.c',
    #'clear_cache_test.c',
    'clzdi2_test.c',
    'clzsi2_test.c',
    'clzti2_test.c',
    'cmpdi2_test.c',
    'cmpti2_test.c',
    'comparedf2_test.c',
    'comparesf2_test.c',
    'compiler_rt_logbf_test.c',
    'compiler_rt_logbl_test.c',
    'compiler_rt_logb_test.c',
    'cpu_model_test.c',
    'ctzdi2_test.c',
    'ctzsi2_test.c',
    'ctzti2_test.c',
    'divdc3_test.c',
    'divdf3_test.c',
    #'divdf3vfp_test.c',
    'divdi3_test.c',
    'divmodsi4_test.c',
    'divsc3_test.c',
    'divsf3_test.c',
    #'divsf3vfp_test.c',
    'divsi3_test.c',
    'divtc3_test.c',
    'divtf3_test.c',
    'divti3_test.c',
    'divxc3_test.c',
    #'enable_execute_stack_test.c',
    #'eqdf2vfp_test.c',
    #'eqsf2vfp_test.c',
    'eqtf2_test.c',
    'extenddftf2_test.c',
    'extendhfsf2_test.c',
    #'extendsfdf2vfp_test.c',
    'extendsftf2_test.c',
    'ffsdi2_test.c',
    'ffssi2_test.c',
    'ffsti2_test.c',
    'fixdfdi_test.c',
    #'fixdfsivfp_test.c',
    'fixdfti_test.c',
    'fixsfdi_test.c',
    #'fixsfsivfp_test.c',
    'fixsfti_test.c',
    'fixtfdi_test.c',
    'fixtfsi_test.c',
    'fixtfti_test.c',
    'fixunsdfdi_test.c',
    'fixunsdfsi_test.c',
    #'fixunsdfsivfp_test.c',
    'fixunsdfti_test.c',
    'fixunssfdi_test.c',
    'fixunssfsi_test.c',
    #'fixunssfsivfp_test.c',
    'fixunssfti_test.c',
    'fixunstfdi_test.c',
    'fixunstfsi_test.c',
    'fixunstfti_test.c',
    'fixunsxfdi_test.c',
    'fixunsxfsi_test.c',
    'fixunsxfti_test.c',
    'fixxfdi_test.c',
    'fixxfti_test.c',
    'floatdidf_test.c',
    'floatdisf_test.c',
    'floatditf_test.c',
    'floatdixf_test.c',
    #'floatsidfvfp_test.c',
    #'floatsisfvfp_test.c',
    'floatsitf_test.c',
    'floattidf_test.c',
    'floattisf_test.c',
    'floattitf_test.c',
    'floattixf_test.c',
    'floatundidf_test.c',
    'floatundisf_test.c',
    'floatunditf_test.c',
    'floatundixf_test.c',
    'floatunsitf_test.c',
    #'floatunssidfvfp_test.c',
    #'floatunssisfvfp_test.c',
    'floatuntidf_test.c',
    'floatuntisf_test.c',
    'floatuntitf_test.c',
    'floatuntixf_test.c',
    #'gcc_personality_test.c',
    #'gedf2vfp_test.c',
    #'gesf2vfp_test.c',
    'getf2_test.c',
    #'gtdf2vfp_test.c',
    #'gtsf2vfp_test.c',
    'gttf2_test.c',
    #'ledf2vfp_test.c',
    #'lesf2vfp_test.c',
    'letf2_test.c',
    'lshrdi3_test.c',
    'lshrsi3_test.c',
    'lshrti3_test.c',
    #'ltdf2vfp_test.c',
    #'ltsf2vfp_test.c',
    'lttf2_test.c',
    'moddi3_test.c',
    'modsi3_test.c',
    'modti3_test.c',
    'muldc3_test.c',
    #'muldf3vfp_test.c',
    'muldi3_test.c',
    'mulodi4_test.c',
    'mulosi4_test.c',
    'muloti4_test.c',
    'mulsc3_test.c',
    #'mulsf3vfp_test.c',
    'mulsi3_test.c',
    'multc3_test.c',
    'multf3_test.c',
    'multi3_test.c',
    'mulvdi3_test.c',
    'mulvsi3_test.c',
    'mulvti3_test.c',
    'mulxc3_test.c',
    #'nedf2vfp_test.c',
    #'negdf2vfp_test.c',
    'negdi2_test.c',
    #'negsf2vfp_test.c',
    'negti2_test.c',
    'negvdi2_test.c',
    'negvsi2_test.c',
    'negvti2_test.c',
    #'nesf2vfp_test.c',
    'netf2_test.c',
    'paritydi2_test.c',
    'paritysi2_test.c',
    'parityti2_test.c',
    'popcountdi2_test.c',
    'popcountsi2_test.c',
    'popcountti2_test.c',
    'powidf2_test.c',
    'powisf2_test.c',
    'powitf2_test.c',
    'powixf2_test.c',
    #'subdf3vfp_test.c',
    #'subsf3vfp_test.c',
    'subtf3_test.c',
    'subvdi3_test.c',
    'subvsi3_test.c',
    'subvti3_test.c',
    'trampoline_setup_test.c',
    'truncdfhf2_test.c',
    'truncdfsf2_test.c',
    #'truncdfsf2vfp_test.c',
    'truncsfhf2_test.c',
    'trunctfdf2_test.c',
    'trunctfsf2_test.c',
    'ucmpdi2_test.c',
    'ucmpti2_test.c',
    'udivdi3_test.c',
    'udivmoddi4_test.c',
    'udivmodsi4_test.c',
    'udivmodti4_test.c',
    'udivsi3_test.c',
    'udivti3_test.c',
    'umoddi3_test.c',
    'umodsi3_test.c',
    'umodti3_test.c',
    #'unorddf2vfp_test.c',
    #'unordsf2vfp_test.c',
    'unordtf2_test.c',
]


if __name__ == '__main__':
    with tempfile.TemporaryDirectory() as tmpdir:
        tests = sys.argv[1:]
        if not tests:
            tests = default_tests
        run_tests(tests, tmpdir)
