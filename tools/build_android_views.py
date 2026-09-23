"""Compile the thin Android View adapter; the application/canvas remains native."""
import argparse
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--android-jar', default=os.environ.get('ANDROID_JAR'))
p.add_argument('--d8-jar', default=os.environ.get('D8_JAR'))
args = p.parse_args()
if not args.android_jar or not args.d8_jar:
    p.error('Pass --android-jar (SDK platform 36) and --d8-jar (SDK build tools), or set ANDROID_JAR/D8_JAR.')
classes = ROOT / 'build' / 'android-classes'
dex = ROOT / 'build' / 'android-dex'
classes.mkdir(parents=True, exist_ok=True)
dex.mkdir(parents=True, exist_ok=True)
sources = sorted((ROOT / 'src' / 'java').rglob('*.java'))
subprocess.run(['javac', '-encoding', 'UTF-8', '-source', '8', '-target', '8',
                '-classpath', args.android_jar, '-d', str(classes), *map(str,sources)], check=True)
subprocess.run(['java', '-cp', args.d8_jar, 'com.android.tools.r8.D8', '--release',
                '--min-api', '26', '--lib', args.android_jar, '--output', str(dex),
                *map(str,sorted(classes.rglob('*.class')))], check=True)
print('Built build/android-dex/classes.dex')
