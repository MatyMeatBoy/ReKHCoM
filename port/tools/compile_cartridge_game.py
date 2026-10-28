"""Compiler launcher for scalar-i32 cartridge arithmetic in game C units only.

The producer supplies explicit -c SOURCE, -o OBJECT and -MF DEPFILE arguments.
Other arguments, including response files, remain opaque to the real compiler.
Use target-scoped producer rules; this adapter must not wrap hosts or libraries.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from cartridge_division_ir import fingerprint as helper_fingerprint, lower_cartridge_division


def fingerprint():
    own = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    return hashlib.sha256((helper_fingerprint()+':'+own).encode()).hexdigest()


def argument_value(arguments, option):
    if arguments.count(option) != 1:
        raise ValueError(f'Expected one explicit {option}; source/output/dependency options cannot be hidden in response files')
    index = arguments.index(option)+1
    if index >= len(arguments):
        raise ValueError(f'Missing value for {option}')
    return index


def compile_game(command, expected):
    if expected != fingerprint():
        raise ValueError('Cartridge compiler fingerprint changed; regenerate the producer build graph')
    if len(command) < 2:
        raise ValueError('Expected compiler and explicit compile arguments')
    compiler, arguments = command[0], list(command[1:])
    source_index = argument_value(arguments, '-c')
    output_index = argument_value(arguments, '-o')
    dependency_index = argument_value(arguments, '-MF')
    source = Path(arguments[source_index])
    if source.suffix.lower() != '.c' or not source.is_file():
        raise ValueError('Cartridge launcher accepts an existing C game source only')
    output, dependency = Path(arguments[output_index]), Path(arguments[dependency_index])
    output.parent.mkdir(parents=True, exist_ok=True)
    dependency.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='rekhcom-game-ir-') as folder:
        folder = Path(folder)
        frontend, guarded = folder/'frontend.ll', folder/'guarded.ll'
        header_dependencies, backend_dependencies = folder/'headers.d', folder/'backend.d'
        object_file = folder/'game.o'
        first = arguments.copy()
        first[output_index], first[dependency_index] = str(frontend), str(header_dependencies)
        if '-MT' not in arguments and '-MQ' not in arguments:
            # The original driver's default dependency target was OBJECT,
            # not our temporary IR output (the armhf Ninja rule uses this).
            first.extend(['-MT', str(output)])
        # Keep original optimization/target/security/ABI flags. O0 would add
        # optnone; disabling LLVM passes preserves division before C UB folds.
        subprocess.run([compiler, *first, '-Xclang', '-disable-llvm-passes', '-S', '-emit-llvm'], check=True)
        guarded.write_text(lower_cartridge_division(frontend.read_text()), encoding='utf-8')
        second = arguments.copy()
        second[source_index], second[output_index] = str(guarded), str(object_file)
        # Do not overwrite the C frontend's real header dependency graph with
        # the backend's IR-only dependencies; no target/ABI flags are guessed.
        second[dependency_index] = str(backend_dependencies)
        subprocess.run([compiler, *second], check=True)
        if not object_file.is_file() or not header_dependencies.is_file():
            raise RuntimeError('Compiler did not produce the expected object/header dependencies')
        # Never return success with an old object after a compiler/lowering
        # failure. Both compiler stages finish before touching existing output.
        pending = output.with_name(output.name+'.cartridge-new')
        shutil.copyfile(object_file, pending)
        os.replace(pending, output)
        pending = dependency.with_name(dependency.name+'.cartridge-new')
        shutil.copyfile(header_dependencies, pending)
        os.replace(pending, dependency)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--print-fingerprint', action='store_true')
    parser.add_argument('--fingerprint')
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.print_fingerprint:
        print(fingerprint())
        return
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    try:
        compile_game(command, args.fingerprint)
    except (ValueError, RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(f'Cartridge game compile failed: {error}', file=sys.stderr)
        raise SystemExit(1)


if __name__ == '__main__':
    main()
