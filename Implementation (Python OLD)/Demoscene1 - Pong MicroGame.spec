# -*- mode: python ; coding: utf-8 -*-

block_cipher = None


a = Analysis(['Demoscene1 - Pong MicroGame.py'],
             pathex=['C:\\Users\\elkek\\Desktop\\Master\\Efectos visuales y Sonoros\\PEC 1 Demoscene\\Implementation'],
             binaries=[],
             datas=[],
             hiddenimports=[],
             hookspath=[],
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher,
             noarchive=False)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          [],
          name='Demoscene1 - Pong MicroGame',
          debug=False,
          bootloader_ignore_signals=False,
          strip=False,
          upx=True,
          upx_exclude=[],
          runtime_tmpdir=None,
          console=True )
