\page cpubrand cpubrand command
``
cpuid []leaf number]
````
Retrieve CPUID information from the processor. See The documentation of CPUID for more information.

![image](https://github.com/brainboxdotcc/retro-rocket/assets/1556794/523b5f5b-510e-4a91-a210-71eae0021fb4)

## CPUID leaf values

<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr align=center>
  <td width="11%" bgcolor="#004080" valign=top rowspan=4><font face="Arial" color="#FFFFFF">0000_xxxxh<br>(standard)</font></td>
  <td width="1%" bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0000h">0000h</a><br>max + ID</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0001h">0001h</a><br>FMS + flags</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0002h">0002h</a><br>caches (v1)</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0003h">0003h</a><br>PSN</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0004h">0004h</a><br>caches (v2)</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0005h">0005h</a><br>MON</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0006h">0006h</a><br>power mgmt</font></td>
  <td width="11%"><font face="Arial"><a href="cpuid.htm#level_0000_0007h">0007h</a><br>flags</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial" color="#808080">0008h<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0009h">0009h</a><br>DCA</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_000Ah">000Ah</a><br>PeMo</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_000Bh">000Bh</a><br>topology</font></td>
  <td><font face="Arial" color="#808080">000Ch<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_000Dh">000Dh</a><br>X state</font></td>
  <td><font face="Arial" color="#808080">000Eh<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_000Fh">000Fh</a><br>PQM</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0010h">0010h</a><br>PQE</font></td>
  <td><font face="Arial" color="#808080">0011h<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0012h">0012h</a><br>SGX</font></td>
  <td><font face="Arial" color="#808080">0013h<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0014h">0014h</a><br>PT</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0015h">0015h</a><br>frequency</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0016h">0016h</a><br>frequency</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0017h">0017h</a><br>attributes</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_0018h">0018h</a><br>TLB</font></td>
  <td><font face="Arial" color="#808080">0019h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">001Ah<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_0000_001Bh">001Bh</a><br>PCONFIG</font></td>
  <td><font face="Arial" color="#808080">001Ch<br>reserved</font></td>
  <td><font face="Arial" color="#808080">001Dh<br>reserved</font></td>
  <td><font face="Arial" color="#808080">001Eh<br>reserved</font></td>
  <td><font face="Arial" color="#808080">001Fh<br>reserved</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080" valign=top rowspan=1><font face="Arial" color="#FFFFFF">2000_xxxxh<br>(Xeon Phi)</font></td>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_2000_0000h">0000h</a><br>max</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_2000_0001h">0001h</a><br>flags</font></td>
  <td><font face="Arial" color="#808080">0002h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0003h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0004h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0005h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0006h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0007h<br>reserved</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080" valign=top rowspan=1><font face="Arial" color="#FFFFFF">4000_xxxxh<br>(hypervisor)</font></td>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0000h">0000h</a><br>vendor</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0001h">0001h</a><br>interface</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0002h">0002h</a><br>version</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0003h">0003h</a><br>features</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0004h">0004h</a><br>recomm.</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0005h">0005h</a><br>limits</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_4000_0006h">0006h</a><br>hardware</font></td>
  <td><font face="Arial" color="#808080">0007h<br>reserved</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080" valign=top rowspan=4><font face="Arial" color="#FFFFFF">8000_xxxxh<br>(extended)</font></td>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0000h">0000h</a><br>max + ID</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0001h">0001h</a><br>FMS + flags</font></td>
  <td colspan=3><font face="Arial"><a href="cpuid.htm#level_8000_0002h">0002h</a> and <a href="cpuid.htm#level_8000_0003h">0003h</a> and <a href="cpuid.htm#level_8000_0004h">0004h</a><br>processor name string</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0005h">0005h</a><br>L1 (v1)</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0006h">0006h</a><br>L2/L3 (v1)</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0007h">0007h</a><br>capabilities</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0008h">0008h</a><br>addr + misc</font></td>
  <td><font face="Arial" color="#808080">0009h<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_000Ah">000Ah</a><br>SVM</font></td>
  <td><font face="Arial" color="#808080">000Bh<br>reserved</font></td>
  <td><font face="Arial" color="#808080">000Ch<br>reserved</font></td>
  <td><font face="Arial" color="#808080">000Dh<br>reserved</font></td>
  <td><font face="Arial" color="#808080">000Eh<br>reserved</font></td>
  <td><font face="Arial" color="#808080">000Fh<br>reserved</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial" color="#808080">0010h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0011h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0012h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0013h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0014h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0015h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0016h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0017h<br>reserved</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial" color="#808080">0018h<br>reserved</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_0019h">0019h</a><br>1G TLB</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_001Ah">001Ah</a><br>perf hints</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_001Bh">001Bh</a><br>IBS</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_001Ch">001Ch</a><br>LWP</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_001Dh">001Dh</a><br>caches (v2)</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_001Eh">001Eh</a><br>topology</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8000_001Fh">001Fh</a><br>SME/SEV</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080" valign=top rowspan=1><font face="Arial" color="#FFFFFF">8086_xxxxh<br>(Transmeta)</font></td>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8086_0000h">0000h</a><br>max + ID</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8086_0001h">0001h</a><br>FMS + flags</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8086_0002h">0002h</a><br>HW/SW rev</font></td>
  <td colspan=4><font face="Arial"><a href="cpuid.htm#level_8086_0003h">0003h</a> and <a href="cpuid.htm#level_8086_0004h">0004h</a> and <a href="cpuid.htm#level_8086_0005h">0005h</a> and <a href="cpuid.htm#level_8086_0006h">0006h</a><br>CMS info string</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_8086_0007h">0007h</a><br>MHz + mV</font></td>
 </tr>
 <tr align=center>
  <td bgcolor="#004080" valign=top rowspan=1><font face="Arial" color="#FFFFFF">C000_xxxxh<br>(Centaur)</font></td>
  <td bgcolor="#004080"><font face="Arial">&nbsp;<br>&nbsp;<br>&nbsp;</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_C000_0000h">0000h</a><br>max + ID</font></td>
  <td><font face="Arial"><a href="cpuid.htm#level_C000_0001h">0001h</a><br>FMS + flags</font></td>
  <td><font face="Arial" color="#808080">0002h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0003h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0004h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0005h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0006h<br>reserved</font></td>
  <td><font face="Arial" color="#808080">0007h<br>reserved</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<a name="level_0000_0000h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0000h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=0000_0000h</font></td>
  <td align=left colspan=2><font face="Arial">get maximum supported standard level and vendor ID string</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">maximum supported standard level <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=11><font face="Arial">EBX-EDX-ECX</font></td>
  <td align=left colspan=2><font face="Arial">vendor ID string <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center width="18%"><tt><b>GenuineIntel</b></tt></td>
  <td width="58%" align=left><font face="Arial">Intel processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>UMC UMC UMC&nbsp;</b></tt></td>
  <td align=left><font face="Arial">UMC processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>AuthenticAMD</b></tt></td>
  <td align=left><font face="Arial">AMD processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>CyrixInstead</b></tt></td>
  <td align=left><font face="Arial">Cyrix processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>NexGenDriven</b></tt></td>
  <td align=left><font face="Arial">NexGen processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>CentaurHauls</b></tt></td>
  <td align=left><font face="Arial">Centaur processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>RiseRiseRise</b></tt></td>
  <td align=left><font face="Arial">Rise Technology processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>SiS SiS SiS&nbsp;</b></tt></td>
  <td align=left><font face="Arial">SiS processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>GenuineTMx86</b></tt></td>
  <td align=left><font face="Arial">Transmeta processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>Geode by NSC</b></tt></td>
  <td align=left><font face="Arial">National Semiconductor processor</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">According to <a href="ftp://ftp.intel.com/pub/IAL/pentium/p5asm.mac" target="_blank">[1]</a> and <a href="ftp://ftp.intel.com/pub/IAL/pentium/p5masm.mac" target="_blank">[2]</a> the pre-B0 step Intel P5 processors return EAX=0000_05xxh.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">According to <a href="ftp://ftp.intel.com/pub/IAL/pentium/p5asm.mac" target="_blank">[1]</a> and <a href="ftp://ftp.intel.com/pub/IAL/pentium/p5masm.mac" target="_blank">[2]</a> the pre-B0 step Intel P5 processors don't return a vendor ID string.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0001h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=6 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0001h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0001h</font></td>
  <td align=left colspan=4><font face="Arial">get processor type/family/model/stepping and feature flags</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td width=50 valign=top align=center rowspan=336 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td width=160 valign=top align=center rowspan=237><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left colspan=4><font face="Arial">processor type/family/model/stepping</font></td>
 </tr>
 <tr>
  <td width=160 valign=top align=center rowspan=11 rowspan=1 bgcolor="#004080"><font color="#FFFFFF" face="Arial">
   extended family<br>
   (add)
  </font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The extended processor family is encoded in bits 27...20.</font></td>
 </tr>
 <tr>
  <td width=160 align=center rowspan=10></td>
  <td width=50 valign=top align=center><font face="Arial">00<font color="#808080">+F</font></font></td>
  <td width=296 align=left><font face="Arial">
   Intel P4<br>
   AMD K8 (Fam 08h)<br>
   Transmeta Efficeon
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">01<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">
   AMD K8L (Fam 10h)<br>
   Intel Itanium 2 (IA-64)
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">02<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD K8 (Fam 11h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">03<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD K8L (Fam 12h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">05<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD BC (Fam 14h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">06<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD BD (Fam 15h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">07<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD JG (Fam 16h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">08<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD ZN (Fam 17h)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">02<font color="#808080">+0</font></font></td>
  <td align=left><font face="Arial">
   Intel Itanium 2 DC (IA-64)<br>
   Intel Itanium 2 QC (IA-64)
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">02<font color="#808080">+1</font></font></td>
  <td align=left><font face="Arial">Intel Itanium 2 8C (IA-64)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=20 bgcolor="#004080"><font color="#FFFFFF" face="Arial">
   extended model<br>
   (concat)
  </font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The extended processor model is encoded in bits 19...16.</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">AMD K8</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">130 nm Rev C</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">90 nm Rev D</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">90 nm Rev E</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">90 nm Rev F</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">90 nm Rev F</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">65 nm Rev G</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">65 nm Rev G</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">C</font></td>
  <td align=left><font face="Arial">90 nm Rev F (in Fr3)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">AMD Fam 15h</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">OR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">TN/RL</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">KV/GV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">CZ/BR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">ST</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">AMD Fam 16h</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">KB/BV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">ML</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">NL</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">AMD Fam 17h</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">ZP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">RV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Intel</font></td>
  <td align=center><font face="Arial">1...F</font></td>
  <td align=left><font face="Arial">see model (below)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5 bgcolor="#004080"><font color="#FFFFFF" face="Arial">type</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The processor type is encoded in bit 13 and bit 12.</font></td>
 </tr>
 <tr>
  <td align=center rowspan=4></td>
  <td align=center><font face="Arial">11b</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10b</font></td>
  <td align=left><font face="Arial">secondary processor (for MP)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">01b</font></td>
  <td align=left><font face="Arial">Overdrive processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">00b</font></td>
  <td align=left><font face="Arial">primary processor</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080"><font color="#FFFFFF" face="Arial">family</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The family is encoded in bits 11...8.</font></td>
 </tr>
 <tr>
  <td align=center rowspan=7></td>
  <td align=center valign=top><font face="Arial">4</font></td>
  <td align=left><font face="Arial">
   most 80486s<br>
   AMD 5x86<br>
   Cyrix 5x86<br>
  </font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">5</font></td>
  <td align=left><font face="Arial">
   Intel P5, P54C, P55C, P24T<br>
   Intel Quark X1000<br>
   NexGen Nx586<br>
   Cyrix M1<br>
   Cyrix MediaGX<br>
   Geode<br>
   AMD K5, K6<br>
   Centaur C6, C2, C3<br>
   Rise mP6<br>
   SiS 55x<br>
   Transmeta Crusoe<br>
  </font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">6</font></td>
  <td align=left><font face="Arial">
   Intel P6, P2, P3, PM, Core 2<br>
   Intel Atom<br>
   Intel Xeon Phi (KNL)<br>
   AMD K7<br>
   Cyrix M2<br>
   VIA C3<br>
  </font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">7</font></td>
  <td align=left><font face="Arial">
   Intel Itanium (IA-64)
  </font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">B</font></td>
  <td align=left><font face="Arial">
   Intel Xeon Phi (KNF and KNC)
  </font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">F</font></td>
  <td align=left><font face="Arial">
   refer to extended family
  </font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">0</font></td>
  <td align=left><font face="Arial">
   refer to extended family
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=190 bgcolor="#004080"><font color="#FFFFFF" face="Arial">model</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The model is encoded in bits 7...4.</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9><font face="Arial">Intel 80486</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">i80486DX-25/33</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">i80486DX-50</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">i80486SX</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">i80486DX2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">i80486SL</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">i80486SX2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">i80486DX2WB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">i80486DX4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">i80486DX4WB</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">UMC 80486</font></td>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">U5D</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">U5S</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=7><font face="Arial">AMD 80486</font></td>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">80486DX2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">80486DX2WB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">80486DX4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">80486DX4WB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">Elan SC400</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">E</font></td>
  <td align=left><font face="Arial">5x86</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">F</font></td>
  <td align=left><font face="Arial">5x86WB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Cyrix 5x86</font></td>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">5x86</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Cyrix MediaGX</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">GX, GXm</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=7><font face="Arial">Intel P5-core</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">P5 A-step</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">P5</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">P54C</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">P24T Overdrive</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">P55C</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">P54C</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">P55C (0.25&micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Intel Quark</font></td>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">X1000</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">NexGen Nx586</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Nx586 or Nx586FPU (only later ones)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Cyrix M1</font></td>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">6x86</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Cyrix M2</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">6x86MX</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">Geode</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">GX1, GXLV, GXm</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">GX2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">LX</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">AMD K5</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">SSA5 (PR75, PR90, PR100)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">5k86 (PR120, PR133)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">5k86 (PR166)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">5k86 (PR200)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">AMD K6</font></td>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">K6 (0.30 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">K6 (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">K6-2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">K6-III</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D</font></td>
  <td align=left><font face="Arial">K6-2+ or K6-III+ (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">Centaur</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">C6</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">C2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">C3</font></td>
 </tr>
 <tr>
  <td rowspan=8 valign=top align=center><font face="Arial">VIA C3</font></td>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">Cyrix M2 core</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">WinChip C5A core</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">WinChip C5B core (if stepping = 0...7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">WinChip C5C core (if stepping = 8...F)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">WinChip C5N core (if stepping = 0...7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">WinChip C5XL core (if stepping = 0...7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">WinChip C5P core (if stepping = 8...F)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">WinChip C5J core</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">Rise</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">mP6 (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">mP6 (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">SiS</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">55x</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Crusoe</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">TM3x00 and TM5x00</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=51><font face="Arial">Intel P6-core</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">P6 A-step</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">P6</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">P2 (0.28 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">P2 (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">P2 with on-die L2 cache</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">P3 (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">P3 (0.18 &micro;m) with 256 KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">P3 (0.18 &micro;m) with 2 MB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B</font></td>
  <td align=left><font face="Arial">P3 (0.13 &micro;m) with 512 KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial" size="-1">PM (0.13 &micro;m) with 1 MB on-die L2 (Banias)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D</font></td>
  <td align=left><font face="Arial" size="-1">PM (0.09 &micro;m) with 2 MB on-die L2 (Dothan)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">E</font></td>
  <td align=left><font face="Arial" size="-1">PM DC (65 nm) with 2 MB on-die L2 (Yonah)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>5</font></td>
  <td align=left><font face="Arial" size="-1">EP80579 (65 nm) with 256 KB on-die L2 (Tolapai)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">F</font></td>
  <td align=left><font face="Arial" size="-1">Core 2 2C (65 nm) 4 MB L2 (Merom)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>6</font></td>
  <td align=left><font face="Arial" size="-1">Core 2 1C (65 nm) 1 MB L2 (Merom-L)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>7</font></td>
  <td align=left><font face="Arial" size="-1">Core 2 2C (45 nm) 6 MB L2 (Penryn)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>D</font></td>
  <td align=left><font face="Arial" size="-1">Core 2 6C (45 nm) 3x3 MB L2 + 16 MB L3 (DUN)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>A</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 4C (45 nm) 8 MB L3 QPI (NHM)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>E</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 4C (45 nm) 8 MB L3 PCIe (CFD/LFD/JSF)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>F</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 2C (45 nm) 4 MB L3 GFX (ABD/HVD)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>E</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 8C (45 nm) 24 MB L3 QPI (BEC)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>C</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 6C (32 nm) 12 MB L3 QPI (WSM)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>5</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 2C (32 nm) 4 MB L3 GFX (ARD/CLD)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>F</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 10C (32 nm) 30 MB L3 QPI (WSM-EX)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>A</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 4C (32 nm) 8 MB L3 GPU (SNB-DT)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>D</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 8C (32 nm) 20 MB L3 PCIe (SNB-E[<font size="-2">NPX</font>])</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>A</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 4C (22 nm) 8 MB L3 GPU (IVB-DT)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>E</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 15C (22 nm) 37.5 MB L3 PCIe (IVB-E[<font size="-2">NPX</font>])</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>C</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 4C (22 nm) 8 MB L3 GPU (HSW-DT)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>F</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 18C (22 nm) 45 MB L3 PCIe (HSW-E[<font size="-2">NPX</font>])</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>5</font></td>
  <td align=left><font face="Arial" size="-1">HSW low power</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>6</font></td>
  <td align=left><font face="Arial" size="-1">HSW Crystalwell (4C 6M GPU and 128M eDRAM)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>D</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 2C (14 nm) 4 MB L3 GPU (BDW-DT)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>F</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 24C (14 nm) 60 MB L3 PCIe (BDW-E[<font size="-2">NPX</font>])</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>6</font></td>
  <td align=left><font face="Arial" size="-1">Core 7 8C (14 nm) 12 MB L3 SoC (BDW-DE)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>7</font></td>
  <td align=left><font face="Arial" size="-1">BDW Brystalwell (4C 6M GPU and 128M eDRAM)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>E</font></td>
  <td align=left><font face="Arial" size="-1">SKL Y/U</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>E</font></td>
  <td align=left><font face="Arial" size="-1">SKL S/H</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">8</font>E</font></td>
  <td align=left><font face="Arial" size="-1">KBL Y/U</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">9</font>E</font></td>
  <td align=left><font face="Arial" size="-1">KBL S/H</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">CFL Y/U</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">CFL S/H</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">WHL Y/U</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">WHL S/H</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>5</font></td>
  <td align=left><font face="Arial" size="-1">SKX</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">6</font>6</font></td>
  <td align=left><font face="Arial" size="-1">CNL Y/U</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">CNL S/H</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">ICL Y/U</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">ICL S/H</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">TGL</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial" size="-1">ADL</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12><font face="Arial">Intel Atom</font></td>
  <td align=center><font face="Arial"><font color="#808080">1</font>C</font></td>
  <td align=left><font face="Arial">Atom (45 nm) with 512 KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>6</font></td>
  <td align=left><font face="Arial">Atom (45 nm) with 512 KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>6</font></td>
  <td align=left><font face="Arial">Atom (32 nm) with 512 KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">2</font>7</font></td>
  <td align=left><font face="Arial">Atom (32 nm) with 512 KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>5</font></td>
  <td align=left><font face="Arial">Atom (?? nm) with ??? KB on-die L2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>A</font></td>
  <td align=left><font face="Arial" size="-1">Atom 2C (22 nm) 1 MB L2 + PowerVR (TGR)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>A</font></td>
  <td align=left><font face="Arial" size="-1">Atom 4C (22 nm) 2 MB L2 + PowerVR (ANN)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>7</font></td>
  <td align=left><font face="Arial" size="-1">Atom 4C (22 nm) 2 MB L2 + Intel Gen7 (BYT)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">4</font>C</font></td>
  <td align=left><font face="Arial" size="-1">Atom 4C (14 nm) 2 MB L2 + Intel Gen8 (BSW)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>C</font></td>
  <td align=left><font face="Arial" size="-1">Atom 4C (14 nm) 2 MB L2 + Intel Gen9 (APL)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">7</font>A</font></td>
  <td align=left><font face="Arial" size="-1">Atom 4C (14 nm) 2 MB L2 + Intel Gen9 (GLK)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>D</font></td>
  <td align=left><font face="Arial" size="-1">Atom 4C (28 nm TSMC) 1 MB L2 + Mali (SoFIA)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">Intel Atom</font></td>
  <td align=center><font face="Arial"><font color="#808080">4</font>D</font></td>
  <td align=left><font face="Arial" size="-1">Atom 8C (22 nm) 4 MB L2 (AVN)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>F</font></td>
  <td align=left><font face="Arial" size="-1">Atom 16C (14 nm) 16 MB L2 (DVN)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">AMD K7</font></td>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Athlon (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Athlon (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">Duron (SF core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">Athlon (TB core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">Athlon (PM core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">Duron (MG core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">Athlon (TH/AP core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">Athlon (BT core)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9><font face="Arial">AMD K8 (Fam 08h)</font></td>
  <td align=center><font face="Arial">xx00b</font></td>
  <td align=left><font face="Arial">Socket 754 or Socket S1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">xx01b</font></td>
  <td align=left><font face="Arial">Socket 940 or Socket F1207</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">xx10b</font></td>
  <td align=left><font face="Arial">if Rev CG, then see K8 erratum #108</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">xx11b</font></td>
  <td align=left><font face="Arial">Socket 939 or Socket AM2 or ASB1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">01xxb</font></td>
  <td align=left><font face="Arial">SH (SC 1024 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11xxb</font></td>
  <td align=left><font face="Arial">DH (SC 512 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10xxb</font></td>
  <td align=left><font face="Arial">CH (SC 256 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">00xxb</font></td>
  <td align=left><font face="Arial">JH (DC 1024 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10xxb</font></td>
  <td align=left><font face="Arial">BH (DC 512 KB)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">AMD K8L (Fam 10h)</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Rev A DR (0/1/2=A0/A1/A2)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Rev B DR (0/1/A/2/3=B0/B1/BA/B2/B3)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4/5/6</font></td>
  <td align=left><font face="Arial">Rev C RB/BL/DA (0/1/2/3=C0/C1/C2/C3)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8/9</font></td>
  <td align=left><font face="Arial">Rev D HY SCM/MCM (0/1=D0/D1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">Rev E PH (0=E0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K8 (Fam 11h)</font></td>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">Rev B LG (1=B1)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">AMD K8L (Fam 12h)</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Rev A LN1 (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Rev B LN1 (0=B0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Rev B LN2 (0=B0)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">AMD BC (Fam 14h)</font></td>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Rev B ON (0=B0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Rev C ON (0=C0)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=10><font face="Arial">AMD BD (Fam 15h)</font></td>
  <td align=center><font face="Arial"><font color="#808080">0</font>0</font></td>
  <td align=left><font face="Arial">Rev A OR (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">0</font>1</font></td>
  <td align=left><font face="Arial">Rev B OR (0/1/2=B0/B1/B2)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">0</font>2</font></td>
  <td align=left><font face="Arial">Rev C OR (0=C0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>0</font></td>
  <td align=left><font face="Arial">Rev A TN (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>3</font></td>
  <td align=left><font face="Arial">Rev A RL (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>0</font></td>
  <td align=left><font face="Arial">Rev A KV (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>8</font></td>
  <td align=left><font face="Arial">Rev A GV (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">6</font>0</font></td>
  <td align=left><font face="Arial">Rev A CZ (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial"><font color="#808080">6</font>5</font></td>
  <td align=left><font face="Arial">
   OSVW.ID5=0: Rev A CZ DDR4 (1=A1)<br>
   OSVW.ID5=1: Rev A BR (1=A1)
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">7</font>0</font></td>
  <td align=left><font face="Arial">Rev A ST (0=A0)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=4><font face="Arial">AMD JG (Fam 16h)</font></td>
  <td align=center><font face="Arial"><font color="#808080">0</font>0</font></td>
  <td align=left><font face="Arial">Rev A KB (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">0</font>4</font></td>
  <td align=left><font face="Arial">Rev A BV (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>0</font></td>
  <td align=left><font face="Arial">Rev A ML (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">6</font>0</font></td>
  <td align=left><font face="Arial">Rev A NL (1=A1)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">AMD ZN (Fam 17h)</font></td>
  <td align=center><font face="Arial"><font color="#808080">0</font>0</font></td>
  <td align=left><font face="Arial">Rev A ZP (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>0</font></td>
  <td align=left><font face="Arial">Rev A RV (1=A1)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">Intel P4-core</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">P4 (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">P4 (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">P4 (0.13 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">P4 (0.09 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">P4 (0.09 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">P4 (65 nm)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">Intel Xeon Phi</font></td>
  <td align=center><font face="Arial">?</font></td>
  <td align=left><font face="Arial">32C (45 nm) 8 MB L2 (KNF) (L1OM)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">62C (22 nm) 31 MB L2 (KNC) (K1OM)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">5</font>7</font></td>
  <td align=left><font face="Arial">72C (14 nm) 36 MB L2 (KNL) (AVX512)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">8</font>5</font></td>
  <td align=left><font face="Arial">72C (14 nm) 36 MB L2 (KNM) (AVX512+)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">?</font>?</font></td>
  <td align=left><font face="Arial">??C (10 nm) ?? MB L2 (KNH)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">Transmeta Efficeon</font></td>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">TM8000 (130 nm)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">TM8000 (90 nm CMS 6.0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">TM8000 (90 nm CMS 6.1+)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">Intel Itanium</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Merced (180 nm)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">Intel Itanium 2</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">McKinley (180 nm)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Madison or Deerfield (130 nm)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Madison 9M (130 nm)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">Intel Itanium 2 DC</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Montecito (90 nm, 9000 series)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Montvale (90 nm, 9100 series)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">Intel Itanium 2 QC</font></td>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Tukwila (65 nm, 9300 series)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">Intel Itanium 2 8C</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Poulson (32 nm, 9500 series)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">stepping</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The stepping is encoded in bits 3...0.</font></td>
 </tr>
 <tr>
  <td align=left colspan=3><font face="Arial">The stepping values are processor-specific.</font></td>
 </tr>
 <tr>
  <td rowspan=33 align=center valign=top><font face="Arial">EBX=aall_ccbbh</font></td>
  <td rowspan=30 align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">brand ID</font></td>
  <td colspan=3 align=left bgcolor="#004080"><font color="#FFFFFF" face="Arial">The brand ID is encoded in bits 7...0.</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">00h</font></td>
  <td colspan=2 align=left><font face="Arial">not supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">01h</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Celeron</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">02h</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Pentium III</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">03h</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Pentium III Xeon</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">03h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Celeron</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">04h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium III</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">07h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Celeron mobile</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">06h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium III mobile</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Ah</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Celeron 4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">08h</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Pentium 4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">09h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium 4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Eh</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Pentium 4 Xeon</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Bh</font></td>
  <td colspan=2 align=left><font face="Arial">0.18 &micro;m Intel Pentium 4 Xeon MP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Bh</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium 4 Xeon</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Ch</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium 4 Xeon MP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">08h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Celeron 4 mobile (0F24h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Fh</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Celeron 4 mobile (0F27h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Eh</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium 4 mobile (production)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Fh</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium 4 mobile (samples)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11h</font></td>
  <td colspan=2 align=left><font face="Arial">mobile Intel ??? processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Celeron M</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12h</font></td>
  <td colspan=2 align=left><font face="Arial">0.09 &micro;m Intel Celeron M</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13h</font></td>
  <td colspan=2 align=left><font face="Arial">mobile Intel Celeron ? processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14h</font></td>
  <td colspan=2 align=left><font face="Arial">Intel Celeron ? processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15h</font></td>
  <td colspan=2 align=left><font face="Arial">mobile Intel ??? processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16h</font></td>
  <td colspan=2 align=left><font face="Arial">0.13 &micro;m Intel Pentium M</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16h</font></td>
  <td colspan=2 align=left><font face="Arial">0.09 &micro;m Intel Pentium M</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17h</font></td>
  <td colspan=2 align=left><font face="Arial">mobile Intel Celeron ? processr</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD</font></td>
  <td colspan=2 align=left><font face="Arial">
   see extended level 8000_0001h<br>
   <font size="-2">with ID=0000_0765_0000_0000b and NN=4_3210b</font>
  </font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">CLFLUSH</font></td>
  <td colspan=3 align=left bgcolor="#004080"><font color="#FFFFFF" face="Arial">The CLFLUSH (8-byte) chunk count is encoded in bits 15...8.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">CPU count</font></td>
  <td colspan=3 align=left bgcolor="#004080"><font color="#FFFFFF" face="Arial">The logical processor count is encoded in bits 23...16.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">APIC ID</font></td>
  <td colspan=3 align=left bgcolor="#004080"><font color="#FFFFFF" face="Arial">The (fixed) default APIC ID is encoded in bits 31...24.</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=center bgcolor="#004080"><a name="level_flags"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 31 (HV)</font></td>
  <td align=left colspan=3><font face="Arial">hypervisor present (and intercepting this bit, to advertise its presence)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 30 (RDRAND)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">RDRAND</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 29 (F16C)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">VCVTPH2PS</a> and <a href="opc_3.htm">VCVTPS2PH</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 28 (AVX)</font></td>
  <td align=left colspan=3><font face="Arial">AVX</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 27 (OSXSAVE)</font></td>
  <td align=left colspan=3><font face="Arial">non-privileged read-only copy of current <a href="crx.htm">CR4.OSXSAVE</a> value</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 26 (XSAVE)</font></td>
  <td align=left colspan=3>
   <font face="Arial"><a href="crx.htm">CR4.OSXSAVE</a>, <a href="crx.htm">XCRn</a>, <a href="opc_grp.htm">XGETBV</a>, <a href="opc_grp.htm">XSETBV</a>, <a href="opc_grp.htm">XSAVE(OPT)</a>, <a href="opc_grp.htm">XRSTOR</a><br>
   also see standard level 0000_000Dh
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 25 (AES)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">AES*</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 24 (TSCD)</font></td>
  <td align=left colspan=3><font face="Arial">local APIC supports one-shot operation using TSC deadline value</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 23 (POPCNT)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">POPCNT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 22 (MOVBE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">MOVBE</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 21 (x2APIC)</font></td>
  <td align=left colspan=3><font face="Arial">
   x2APIC, <a href="msr.htm">APIC_BASE.EXTD</a>, MSRs 0000_0800h...0000_0BFFh<br>
   64-bit ICR (+030h but not +031h), no DFR (+00Eh), SELF_IPI (+040h)<br>
   also see standard level 0000_000Bh
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 20 (SSE4.2)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">SSE4.2</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 19 (SSE4.1)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">SSE4.1</a>, <a href="fp_new.htm">MXCSR</a>, <a href="crx.htm">CR4.OSXMMEXCPT</a>, <a href="except.htm">#XF</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 18 (DCA)</font></td>
  <td align=left colspan=3><font face="Arial">
   Direct Cache Access (that is, the ability to prefetch data from MMIO)<br>
   also see standard level 0000_0009h
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 17 (PCID)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.PCIDE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 16</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 15 (PDCM)</font></td>
  <td align=left colspan=3><font face="Arial">Performance Debug Capability MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 14 (ETPRD)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MISC_ENABLE.ETPRD</a></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#B0D0D0"><font face="Arial">bit 13 (CX16)</font></td>
  <td align=left colspan=3 bgcolor="#B0D0D0"><font face="Arial">CMPXCHG16B</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 12 (FMA)</font></td>
  <td align=left colspan=3><font face="Arial">FMA</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 11 (SDBG)</font></td>
  <td align=left colspan=3><font face="Arial">DEBUG_INTERFACE MSR for silicon debug</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 10 (CID)</font></td>
  <td align=left colspan=3><font face="Arial">
   context ID: the L1 data cache can be set to adaptive or shared mode<br>
   <a href="msr.htm">MISC_ENABLE.L1DCCM</a>
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 9 (SSSE3)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">SSSE3</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 8 (TM2)</font></td>
  <td align=left colspan=3><font face="Arial">
   <a href="msr.htm">MISC_ENABLE.TM2E</a><br>
   THERM_INTERRUPT and THERM_STATUS MSRs<br>
   xAPIC thermal LVT entry<br>
   THERM2_CONTROL MSR
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 7 (EST)</font></td>
  <td align=left colspan=3><font face="Arial">Enhanced SpeedStep Technology</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 6 (SMX)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.SMXE</a>, <a href="opc_2.htm">GETSEC</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 5 (VMX)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.VMXE</a>, <a href="opc_grp.htm">VM*</a> and <a href="opc_2.htm">VM*</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 4 (DSCPL)</font></td>
  <td align=left colspan=3><font face="Arial">CPL-qualified Debug Store</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 3 (MON)</font></td>
  <td align=left colspan=3><font face="Arial">
   <a href="opc_grp.htm">MONITOR/MWAIT</a>, <a href="msr.htm">MISC_ENABLE.MONE</a>, <a href="msr.htm">MISC_ENABLE.LCMV</a><br>
   MONITOR_FILTER_LINE_SIZE MSR<br>
   also see standard level 0000_0005h<br>
   setting MISC_ENABLE.MONE=0 causes MON=0
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 2 (DTES64)</font></td>
  <td align=left colspan=3><font face="Arial">64-bit Debug Trace and EMON Store MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 1 (PCLMUL)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_3.htm">PCLMULQDQ</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 0 (SSE3)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">SSE3</a>, <a href="fp_new.htm">MXCSR</a>, <a href="crx.htm">CR4.OSXMMEXCPT</a>, <a href="except.htm">#XF</a>, if FPU=1 then also <a href="opc_fpu.htm">FISTTP</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=center bgcolor="#004080"><a name="level_flags"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 31 (PBE)</font></td>
  <td align=left colspan=3><font face="Arial">Pending Break Event, <a href="inter.htm">STPCLK</a>, <a href="legacy.htm">FERR#</a>, <a href="msr.htm">MISC_ENABLE.PBE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 30 (IA-64)</font></td>
  <td align=left colspan=3><font face="Arial">IA-64, <a href="opc_2.htm">JMPE Jv</a>, <a href="opc_grp.htm">JMPE Ev</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 29 (TM1)</font></td>
  <td align=left colspan=3><font face="Arial">
   <a href="msr.htm">MISC_ENABLE.TM1E</a><br>
   THERM_INTERRUPT and THERM_STATUS MSRs<br>
   xAPIC thermal LVT entry
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 28 (HTT)</font></td>
  <td align=left colspan=3><font face="Arial">Hyper-Threading Technology, <a href="opc_1.htm">PAUSE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 27 (SS)</font></td>
  <td align=left colspan=3><font face="Arial">selfsnoop</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 26 (SSE2)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">SSE2</a>, <a href="fp_new.htm">MXCSR</a>, <a href="crx.htm">CR4.OSXMMEXCPT</a>, <a href="except.htm">#XF</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 25 (SSE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">SSE</a>, <a href="fp_new.htm">MXCSR</a>, <a href="crx.htm">CR4.OSXMMEXCPT</a>, <a href="except.htm">#XF</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 24 (FXSR)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">FXSAVE/FXRSTOR</a>, <a href="crx.htm">CR4.OSFXSR</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 23 (MMX)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">MMX</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 22 (ACPI)</font></td>
  <td align=left colspan=3><font face="Arial">THERM_CONTROL MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 21 (DTES)</font></td>
  <td align=left colspan=3><font face="Arial">Debug Trace and EMON Store MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 20</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 19 (CLFL)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">CLFLUSH</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 18 (PSN)</font></td>
  <td align=left colspan=3><font face="Arial">PSN (see standard level 0000_0003h), <a href="msr.htm">MISC_CTL.PSND</a> <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 17 (PSE36)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">4 MB PDE bits 16...13</a>, <a href="crx.htm">CR4.PSE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 16 (PAT)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">PAT MSR</a>, <a href="paging.htm">PDE/PTE.PAT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 15 (CMOV)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">CMOVcc</a>, if FPU=1 then also <a href="opc_fpu.htm">FCMOVcc/F(U)COMI(P)</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 14 (MCA)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MCG_*/MCn_* MSRs</a>, <a href="crx.htm">CR4.MCE</a>, <a href="except.htm">#MC</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 13 (PGE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">PDE/PTE.G</a>, <a href="crx.htm">CR4.PGE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 12 (MTRR)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MTRR* MSRs</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 11 (SEP)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">SYSENTER/SYSEXIT</a>, <a href="msr.htm">SEP_* MSRs</a> <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 10</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 9 (APIC)</font></td>
  <td align=left colspan=3><font face="Arial">APIC <sup>#3, #4</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 8 (CX8)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">CMPXCHG8B</a> <sup>#5</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 7 (MCE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MCAR/MCTR MSRs</a>, <a href="crx.htm">CR4.MCE</a>, <a href="except.htm">#MC</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 6 (PAE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">64-bit PDPTE/PDE/PTEs</a>, <a href="crx.htm">CR4.PAE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 5 (MSR)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MSRs</a>, <a href="opc_2.htm">RDMSR/WRMSR</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 4 (TSC)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">TSC</a>, <a href="opc_2.htm">RDTSC</a>, <a href="crx.htm">CR4.TSD</a> (doesn't imply MSR=1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 3 (PSE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">PDE.PS</a>, <a href="paging.htm">PDE/PTE.res</a>, <a href="crx.htm">CR4.PSE</a>, <a href="except.htm">#PF(1xxxb)</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 2 (DE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.DE</a>, <a href="drx.htm">DR7.RW=10b</a>, <a href="except.htm">#UD</a> on MOV from/to DR4/5</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 1 (VME)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.VME/PVI</a>, <a href="flags.htm">EFLAGS.VIP/VIF</a>, <a href="tss.htm">TSS32.IRB</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 0 (FPU)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_fpu.htm">FPU</a></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=5 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=5><font face="Arial">
   If the PSN has been disabled, then the PSN feature flag will read as 0. In addition the value for the maximum<br>
   supported standard level (reported by standard level 0000_0000h, register EAX) will be lower.
  </font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=5><font face="Arial">The Intel P6 processor does not support SEP, but inadvertently reports it.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#3</font></td>
  <td align=left colspan=5><font face="Arial">If the APIC has been disabled, then the APIC feature flag will read as 0.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#4</font></td>
  <td align=left colspan=5><font face="Arial">Early AMD K5 processors (SSA5) inadvertently used this bit to report PGE support.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#5</font></td>
  <td align=left colspan=5><font face="Arial">Some processors do support CMPXCHG8B, but don't report it by default. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0002h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0002h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=0000_0002h</font></td>
  <td align=left colspan=2><font face="Arial">get processor configuration descriptors</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=145 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">AL</font></td>
  <td align=left colspan=2><font face="Arial">number of times this level must be queried to obtain all configuration descriptors <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=144><font face="Arial">
   EAX.15...8<br>EAX.23...16<br>EAX.31...24<br>
   EBX.0...7<br>EBX.15...8<br>EBX.23...16<br>EBX.31...24<br>
   ECX.0...7<br>ECX.15...8<br>ECX.23...16<br>ECX.31...24<br>
   EDX.0...7<br>EDX.15...8<br>EDX.23...16<br>EDX.31...24<br>
  </font></td>
  <td align=left colspan=2><font face="Arial">configuration descriptors <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">value</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">00h</font></td>
  <td align=left><font face="Arial">null descriptor (=unused descriptor)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">01h</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, 4 ways, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">02h</font></td>
  <td align=left><font face="Arial">code TLB, 4M pages, fully, 2 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">03h</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, 4 ways, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">04h</font></td>
  <td align=left><font face="Arial">data TLB, 4M pages, 4 ways, 8 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">05h</font></td>
  <td align=left><font face="Arial">data TLB, 4M pages, 4 ways, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">06h</font></td>
  <td align=left><font face="Arial">code L1 cache, 8 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">08h</font></td>
  <td align=left><font face="Arial">code L1 cache, 16 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">09h</font></td>
  <td align=left><font face="Arial">code L1 cache, 32 KB, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Ah</font></td>
  <td align=left><font face="Arial">data L1 cache, 8 KB, 2 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Bh</font></td>
  <td align=left><font face="Arial">code TLB, 4M pages, 4 ways, 4 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Ch</font></td>
  <td align=left><font face="Arial">data L1 cache, 16 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Dh</font></td>
  <td align=left><font face="Arial">data L1 cache, 16 KB, 4 ways, 64 byte lines (ECC)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Eh</font></td>
  <td align=left><font face="Arial">data L1 cache, 24 KB, 6 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10h</font></td>
  <td align=left><font face="Arial">data L1 cache, 16 KB, 4 ways, 32 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15h</font></td>
  <td align=left><font face="Arial">code L1 cache, 16 KB, 4 ways, 32 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1Ah</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 96 KB, 6 ways, 64 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1Dh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 128 KB, 2 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 256 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">22h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 512 KB, 4 ways (!), 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 1024 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">24h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 1024 KB, 16 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 2048 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 4096 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Ch</font></td>
  <td align=left><font face="Arial">data L1 cache, 32 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30h</font></td>
  <td align=left><font face="Arial">code L1 cache, 32 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">39h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 128 KB, 4 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Ah</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 192 KB, 6 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Bh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 128 KB, 2 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Ch</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 256 KB, 4 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Dh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 384 KB, 6 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Eh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 4 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">40h</font></td>
  <td align=left><font face="Arial">no integrated L2 cache (P6 core) or L3 cache (P4 core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">41h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 128 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">42h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 256 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">43h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">44h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 1024 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">45h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 2048 KB, 4 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">46h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 4096 KB, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">47h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 8192 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">48h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 3072 KB, 12 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">49h</font></td>
  <td align=left><font face="Arial">
   code and data L3 cache, 4096 KB, 16 ways, 64 byte lines (P4) or<br>
   code and data L2 cache, 4096 KB, 16 ways, 64 byte lines (Core 2)
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4Ah</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 6144 KB, 12 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4Bh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 8192 KB, 16 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4Ch</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 12288 KB, 12 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4Dh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 16384 KB, 16 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4Eh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 6144 KB, 24 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4Fh</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, ???, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">50h</font></td>
  <td align=left><font face="Arial">code TLB, 4K/4M/2M pages, fully, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">51h</font></td>
  <td align=left><font face="Arial">code TLB, 4K/4M/2M pages, fully, 128 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">52h</font></td>
  <td align=left><font face="Arial">code TLB, 4K/4M/2M pages, fully, 256 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">55h</font></td>
  <td align=left><font face="Arial">code TLB, 2M/4M, fully, 7 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">56h</font></td>
  <td align=left><font face="Arial">L0 data TLB, 4M pages, 4 ways, 16 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">57h</font></td>
  <td align=left><font face="Arial">L0 data TLB, 4K pages, 4 ways, 16 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">59h</font></td>
  <td align=left><font face="Arial">L0 data TLB, 4K pages, fully, 16 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5Ah</font></td>
  <td align=left><font face="Arial">L0 data TLB, 2M/4M, 4 ways, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5Bh</font></td>
  <td align=left><font face="Arial">data TLB, 4K/4M pages, fully, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5Ch</font></td>
  <td align=left><font face="Arial">data TLB, 4K/4M pages, fully, 128 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5Dh</font></td>
  <td align=left><font face="Arial">data TLB, 4K/4M pages, fully, 256 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">60h</font></td>
  <td align=left><font face="Arial">data L1 cache, 16 KB, 8 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">61h</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, fully, 48 entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">63h</font></td>
  <td align=left><font face="Arial">
   data TLB, 2M/4M pages, 4-way, 32-entries, and<br>
   data TLB, 1G pages, 4-way, 4 entries
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">64h</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, 4-way, 512 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">66h</font></td>
  <td align=left><font face="Arial">data L1 cache, 8 KB, 4 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">67h</font></td>
  <td align=left><font face="Arial">data L1 cache, 16 KB, 4 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">68h</font></td>
  <td align=left><font face="Arial">data L1 cache, 32 KB, 4 ways, 64 byte lines, sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6Ah</font></td>
  <td align=left><font face="Arial">L0 data TLB, 4K pages, 8-way, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6Bh</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, 8-way, 256 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6Ch</font></td>
  <td align=left><font face="Arial">data TLB, 2M/4M pages, 8-way, 126 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6Dh</font></td>
  <td align=left><font face="Arial">data TLB, 1G pages, fully, 16 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">70h</font></td>
  <td align=left><font face="Arial">trace L1 cache, 12 K&micro;OPs, 8 ways</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">71h</font></td>
  <td align=left><font face="Arial">trace L1 cache, 16 K&micro;OPs, 8 ways</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">72h</font></td>
  <td align=left><font face="Arial">trace L1 cache, 32 K&micro;OPs, 8 ways</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">73h</font></td>
  <td align=left><font face="Arial">trace L1 cache, 64 K&micro;OPs, 8 ways</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">76h</font></td>
  <td align=left><font face="Arial">code TLB, 2M/4M pages, fully, 8 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">77h</font></td>
  <td align=left><font face="Arial">code L1 cache, 16 KB, 4 ways, 64 byte lines, sectored (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">78h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 1024 KB, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">79h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 128 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7Ah</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 256 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7Bh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7Ch</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 1024 KB, 8 ways, 64 byte lines, dual-sectored</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7Dh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 2048 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7Eh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 256 KB, 8 ways, 128 byte lines, sect. (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7Fh</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 2 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">80h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">81h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 128 KB, 8 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">82h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 256 KB, 8 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">83h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 8 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">84h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 1024 KB, 8 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">85h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 2048 KB, 8 ways, 32 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">86h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 512 KB, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">87h</font></td>
  <td align=left><font face="Arial">code and data L2 cache, 1024 KB, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">88h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 2048 KB, 4 ways, 64 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">89h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 4096 KB, 4 ways, 64 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8Ah</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 8192 KB, 4 ways, 64 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8Dh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 3072 KB, 12 ways, 128 byte lines (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">90h</font></td>
  <td align=left><font face="Arial">code TLB, 4K...256M pages, fully, 64 entries (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">96h</font></td>
  <td align=left><font face="Arial">data L1 TLB, 4K...256M pages, fully, 32 entries (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9Bh</font></td>
  <td align=left><font face="Arial">data L2 TLB, 4K...256M pages, fully, 96 entries (IA-64)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A0h</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, fully, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B0h</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, 4 ways, 128 entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">B1h</font></td>
  <td align=left><font face="Arial">
   code TLB, 4M pages, 4 ways, 4 entries and<br>
   code TLB, 2M pages, 4 ways, 8 entries
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B2h</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, 4 ways, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B3h</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, 4 ways, 128 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B4h</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, 4 ways, 256 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B5h</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, 8 ways, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">B6h</font></td>
  <td align=left><font face="Arial">code TLB, 4K pages, 8 ways, 128 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">BAh</font></td>
  <td align=left><font face="Arial">data TLB, 4K pages, 4 ways, 64 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">C0h</font></td>
  <td align=left><font face="Arial">data TLB, 4K/4M pages, 4 ways, 8 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">C1h</font></td>
  <td align=left><font face="Arial">L2 code and data TLB, 4K/2M pages, 8 ways, 1024 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">C2h</font></td>
  <td align=left><font face="Arial">data TLB, 2M/4M pages, 4 ways, 16 entries</font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">C3h</font></td>
  <td align=left><font face="Arial">
   L2 code and data TLB, 4K/2M pages, 6 ways, 1536 entries and<br>
   L2 code and data TLB, 1G pages, 4 ways, 16 entries
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">C4h</font></td>
  <td align=left><font face="Arial">data TLB, 2M/4M pages, 4-way, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">CAh</font></td>
  <td align=left><font face="Arial">L2 code and data TLB, 4K pages, 4 ways, 512 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D0h</font></td>
  <td align=left><font face="Arial">code and data L3 cache,  512-kb, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D1h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 1024-kb, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D2h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 2048-kb, 4 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D6h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 1024-kb, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D7h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 2048-kb, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D8h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 4096-kb, 8 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">DCh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 1536-kb, 12 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">DDh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 3072-kb, 12 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">DEh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 6144-kb, 12 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">E2h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 2048-kb, 16 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">E3h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 4096-kb, 16 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">E4h</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 8192-kb, 16 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EAh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 12288-kb, 24 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EBh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 18432-kb, 24 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECh</font></td>
  <td align=left><font face="Arial">code and data L3 cache, 24576-kb, 24 ways, 64 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">F0h</font></td>
  <td align=left><font face="Arial">64 byte prefetching</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">F1h</font></td>
  <td align=left><font face="Arial">128 byte prefetching</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">FEh</font></td>
  <td align=left><font face="Arial">query standard level 0000_0018h instead</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">FFh</font></td>
  <td align=left><font face="Arial">query standard level 0000_0004h instead</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">value</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">70h</font></td>
  <td align=left><font face="Arial">Cyrix specific: code and data TLB, 4K pages, 4 ways, 32 entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">74h</font></td>
  <td align=left><font face="Arial">Cyrix specific: ???</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">77h</font></td>
  <td align=left><font face="Arial">Cyrix specific: ???</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">80h</font></td>
  <td align=left><font face="Arial">Cyrix specific: code and data L1 cache, 16 KB, 4 ways, 16 byte lines</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">82h</font></td>
  <td align=left><font face="Arial">Cyrix specific: ???</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">84h</font></td>
  <td align=left><font face="Arial">Cyrix specific: ???</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">value</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">others</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center colspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">example<br>(here: P6)</font></td>
  <td align=center><font face="Arial">
   EAX=0302_0101h<br>
   EBX=0000_0000h<br>
   ECX=0000_0000h<br>
   EDX=0604_0A43h<br>
  </font></td>
  <td align=left><font face="Arial">
   Because AL is 01h, one invocation of the level is enough to obtain all the
   configuration descriptors. All of them are valid because their highest bits
   are 0. This P6 processor includes a 4K/M code/data TLB, an 8+8 KB code/data
   L1 cache and an integrated 512 KB code and data L2 cache.
  </font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">
   In a MP system special precautions must be taken when executing standard level 0000_0002h more than once.<br>
   In particular it must be ensured that the same CPU is used during that entire process.
  </font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">Programs must not expect any particular order for the reported configuration descriptors.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0003h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=3 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0003h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=0000_0003h</font></td>
  <td align=left width="76%"><font face="Arial">get processor serial number <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">processor serial number (Transmeta Efficeon processors only)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EBX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">processor serial number (Transmeta Crusoe and Efficeon processors only)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">processor serial number</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">processor serial number</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=2 width="94%"><font face="Arial">
   This level is only supported and enabled if the PSN feature flag is set. The
   reported processor serial number should be combined with the vendor ID string
   and the processor type/family/model/stepping value, to distinguish cases in
   which two processors from different vendors happen to have the same serial
   number. Finally, it should be noted that most vendors can not guarantee that
   their serial numbers are truely unique.
  </font></td>
 </tr>
</table>
<br>

<a name="level_0000_0004h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0004h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0004h</font></td>
  <td align=left colspan=2><font face="Arial">get cache configuration descriptors <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">cache level to query (e.g. 0=L1D, 1=L2, or 0=L1D, 1=L1I, 2=L2)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=19 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=8 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...26</font></td>
  <td align=left><font face="Arial">cores per package - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25...14</font></td>
  <td align=left><font face="Arial">threads per cache - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13...10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">fully associative?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">self-initializing?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...5</font></td>
  <td align=left><font face="Arial">cache level (starts at 1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">cache type (0=null, 1=data, 2=code, 3=unified, 4...31=reserved)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...22</font></td>
  <td align=left><font face="Arial">ways of associativity - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21...12</font></td>
  <td align=left><font face="Arial">physical line partitions - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">system coherency line size - 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">sets - 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">complex indexing?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">inclusive of lower levels?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">write-back invalidate?</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0005h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0005h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0005h</font></td>
  <td align=left colspan=2><font face="Arial">get MON information <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=19 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=3 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">smallest monitor line size in bytes</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">largest monitor line size in bytes</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...2</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">treat interrupts as break events, even when interrupts are disabled</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">enumeration of MWAIT extensions (beyond EAX and EBX)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...28</font></td>
  <td align=left><font face="Arial">number of C7 sub C-states for MWAIT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27...24</font></td>
  <td align=left><font face="Arial">number of C6 sub C-states for MWAIT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...20</font></td>
  <td align=left><font face="Arial">number of C5 sub C-states for MWAIT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">19...16</font></td>
  <td align=left><font face="Arial">number of C4 sub C-states for MWAIT (starting with Core 7: C7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">number of C3 sub C-states for MWAIT (starting with Core 7: C6)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...8</font></td>
  <td align=left><font face="Arial">number of C2 sub C-states for MWAIT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...4</font></td>
  <td align=left><font face="Arial">number of C1 sub C-states for MWAIT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3...0</font></td>
  <td align=left><font face="Arial">number of C0 sub C-states for MWAIT</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0006h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0006h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0006h</font></td>
  <td align=left colspan=2><font face="Arial">get power management information <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=32 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=21 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...19</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">18</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13 (HDC)</font></td>
  <td align=left><font face="Arial">PKG_HDC_CTL, PM_CTL1, and THREAD_STALL MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11 (HWP_PLR)</font></td>
  <td align=left><font face="Arial">HWP_REQUEST_PKG MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10 (HWP_EPP)</font></td>
  <td align=left><font face="Arial">HWP_REQUEST MSR bits 31...24</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9 (HWP_ACT)</font></td>
  <td align=left><font face="Arial">HWP_REQUEST MSR bits 41...32</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8 (HWP_NOT)</font></td>
  <td align=left><font face="Arial">HWP_INTERRUPT MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7 (HWP)</font></td>
  <td align=left><font face="Arial">PM_ENABLE bit 0, and HWP_{CAPABILITIES,REQUEST,STATUS} MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6 (PTM)</font></td>
  <td align=left><font face="Arial">PACKAGE_THERMAL_STATUS MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5 (ECMD)</font></td>
  <td align=left><font face="Arial">CLOCK_MODULATION MSR</font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">4 (PLN)</font></td>
  <td align=left><font face="Arial">
   THERM_STATUS MSR bits 10/11<br>
   THERM_INTERRUPT MSR bit 24
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (ARAT)<br>2 (OPP)</font></td>
  <td align=left><font face="Arial">always running APIC timer (in every C-state and regardless of P-state)<br>P4: operating point protection (protect CPU's ratio/VID points) <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (DA)</font></td>
  <td align=left><font face="Arial">dynamic acceleration (<a href="msr.htm">MISC.ENABLE.DAD=0</a>)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (DTS)</font></td>
  <td align=left><font face="Arial">digital thermal sensor</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3...0</font></td>
  <td align=left><font face="Arial">number of programmable digital thermal sensor interrupt thresholds</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">ENERGY_PERF_BIAS MSR (0000_01B0h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">ACNT2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial"><a href="msr.htm">MPERF/APERF</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">
   The implementation of OPP is processor and stepping specific.<br>
   On certain Pentium 4 processors, the protection mechanism is Snap-to-VID and it is enabled if the bit is set.
  </font></td>
 </tr>
</table>
<br>

<a name="level_0000_0007h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0007h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0007h</font></td>
  <td align=left colspan=2><font face="Arial">get feature flags <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0...n as per EAX reported by sub-level 0)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=101 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-1">(sub 0)</font></font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">maximum supported sub-level</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31 (AVX512VL)</font></td>
  <td align=left><font face="Arial">AVX512VL</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30 (AVX512BW)</font></td>
  <td align=left><font face="Arial">AVX512BW</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29 (SHA)</font></td>
  <td align=left><font face="Arial">SHA</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">28 (AVX512CD)</font></td>
  <td align=left><font face="Arial">AVX512CD</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27 (AVX512ER)</font></td>
  <td align=left><font face="Arial">AVX512ER</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">26 (AVX512PF)</font></td>
  <td align=left><font face="Arial">AVX512PF</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25 (PT)</font></td>
  <td align=left><font face="Arial">processor trace, standard level 0000_0014h</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">24 (CLWB)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">CLWB</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23 (CLFLUSHOPT)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">CLFLUSHOPT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">22 (PCOMMIT)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">PCOMMIT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21 (AVX512IFMA)</font></td>
  <td align=left><font face="Arial">AVX512IFMA</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">20 (SMAP)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">CR4.SMAP</a>, <a href="opc_grp.htm">CLAC</a> and <a href="opc_grp.htm">STAC</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">19 (ADX)</font></td>
  <td align=left><font face="Arial"><a href="opc_3.htm">ADCX</a> and <a href="opc_3.htm">ADOX</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">18 (RDSEED)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">RDSEED</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17 (AVX512DQ)</font></td>
  <td align=left><font face="Arial">AVX512DQ</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16 (AVX512F)</font></td>
  <td align=left><font face="Arial">AVX512F, EVEX, ZMM0...31, K0...7, modifiers, VSIB512, disp8*N</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15 (PQE)</font></td>
  <td align=left><font face="Arial">platform quality of service enforcement</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">14 (MPX)</font></td>
  <td align=left><font face="Arial">
   <a href="crx.htm">XCR0.Breg</a>, <a href="crx.htm">XCR0.BNDCSR</a>, <a href="bnd.htm">BNDCFGS/BNDCFGU/BNDSTATUS</a> and
   <a href="bnd.htm">BND0...BND3</a>, <a href="opc_1.htm">BND:</a>, MPX</font>
  </td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13 (FPCSDS)</font></td>
  <td align=left><font face="Arial"><a href="fp_old.htm">FP_CS</a> and <a href="fp_old.htm">FP_DS</a> always saved as 0000h</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12 (PQM)</font></td>
  <td align=left><font face="Arial">platform quality of service monitoring</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11 (RTM)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">XBEGIN</a>, <a href="opc_grp.htm">XABORT</a>, <a href="opc_grp.htm">XEND</a>, <a href="opc_grp.htm">XTEST</a>, <a href="drx.htm">DR7.RTM</a>, <a href="drx.htm">DR6.RTM</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10 (INVPCID)</font></td>
  <td align=left><font face="Arial"><a href="opc_3.htm">INVPCID</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9 (ERMS)</font></td>
  <td align=left><font face="Arial">enhanced REP MOVSB/STOSB (while <a href="msr.htm">MISC_ENABLE.FSE=1</a>)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8 (BMI2)</font></td>
  <td align=left><font face="Arial">BMI2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7 (SMEP)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">CR4.SMEP</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6 (FPDP)</font></td>
  <td align=left><font face="Arial"><a href="fp_old.htm">FP_DP</a> for non-control instructions only if unmasked exception(s)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5 (AVX2)</font></td>
  <td align=left><font face="Arial">AVX2 (including <a href="opc_sib.htm">VSIB</a>)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4 (HLE)</font></td>
  <td align=left><font face="Arial"><a href="opc_1.htm">XACQUIRE:</a>, <a href="opc_1.htm">XRELEASE:</a>, <a href="opc_grp.htm">XTEST</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (BMI1)</font></td>
  <td align=left><font face="Arial">BMI1 and <a href="opc_2.htm">TZCNT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (SGX)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">CR4.SEE</a>, <a href="msr.htm">PRMRR</a>, <a href="opc_grp.htm">ENCLS</a> and <a href="opc_grp.htm">ENCLU</a>, standard level 0000_0012h</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (TSC_ADJUST)</font></td>
  <td align=left><font face="Arial"><a href="msr.htm">TSC_ADJUST</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (FSGSBASE)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">CR4.FSGSBASE</a> and <a href="opc_grp.htm">[RD|WR][FS|GS]BASE</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30 (SGX_LC)</font></td>
  <td align=left><font face="Arial">SGX launch configuration</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">28</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">26</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">24</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">22 (RDPID)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">RDPID</a>, <a href="msr.htm">TSC_AUX</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21 (MAWAU)</font></td>
  <td align=left><font face="Arial">MPX address-width adjust for CPL=3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">20 (MAWAU)</font></td>
  <td align=left><font face="Arial">MPX address-width adjust for CPL=3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">19 (MAWAU)</font></td>
  <td align=left><font face="Arial">MPX address-width adjust for CPL=3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">18 (MAWAU)</font></td>
  <td align=left><font face="Arial">MPX address-width adjust for CPL=3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17 (MAWAU)</font></td>
  <td align=left><font face="Arial">MPX address-width adjust for CPL=3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16 (VA57)</font></td>
  <td align=left><font face="Arial"><a href="paging.htm">5-level paging</a>, <a href="crx.htm">CR4.VA57</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14 (AVX512VP...DQ)</font></td>
  <td align=left><font face="Arial">VPOPCNT{D,Q}</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13 (TME)</font></td>
  <td align=left><font face="Arial">TME</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12 (AVX512BITALG)</font></td>
  <td align=left><font face="Arial">VPOPCNT{B,W} and VPSHUFBITQMB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11 (AVX512VNNI)</font></td>
  <td align=left><font face="Arial">VPDP{BUS,WSS}D[S]</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10 (VPCL)</font></td>
  <td align=left><font face="Arial">VPCLMULQDQ (VEX.256 and EVEX)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9 (VAES)</font></td>
  <td align=left><font face="Arial">VAES{ENC,DEC}{,LAST} (VEX.256 and EVEX)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8 (GFNI)</font></td>
  <td align=left><font face="Arial">[V]GF2P8AFFINE{,INV}QB and [V]GF2P8MULB (SSE, VEX, and EVEX)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">7 (CET)</font></td>
  <td align=left><font face="Arial">
   <a href="crx.htm">CR4.CET</a>, <a href="msr.htm">XSS.CET_{U,S}</a>, <a href="cet.htm">{U,S}_CET MSRs</a>, <a href="cet.htm">PL{0,1,2,3}_SSP MSRs</a>,<br>
   <a href="cet.htm">IST_SSP MSR</a> and 8-entry interrupt SSP table, <a href="except.htm">#CP</a>, <a href="cet.htm">SSP</a>, <a href="tss.htm">TSS32.SSP</a>,<br>
   <a href="opc_grp.htm">INCSSP</a>, <a href="opc_grp.htm">RDSSP</a>, <a href="opc_grp.htm">SAVESSP</a>, <a href="opc_grp.htm">RSTORSSP</a>, <a href="opc_grp.htm">SETSSBSY</a>, <a href="opc_grp.htm">CLRSSBSY</a>,<br>
   <a href="opc_3.htm">WRSS</a>, <a href="opc_3.htm">WRUSS</a>, <a href="opc_grp.htm">ENDBR32</a>, <a href="opc_grp.htm">ENDBR64</a>, <a href="opc_grp.htm">CALL/JMP Rv</a> + <a href="opc_1.htm">no track (3Eh)</a>
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6 (AVX512VBMI2)</font></td>
  <td align=left><font face="Arial">VP{EXPAND,COMPRESS}{B,W} and VP{SHL,SHR}D{,V}{W,D,Q}</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4 (OSPKE)</font></td>
  <td align=left><font face="Arial">non-privileged read-only copy of current <a href="crx.htm">CR4.PKE</a> value</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (PKU)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">XCR0.PKRU</a>, <a href="crx.htm">CR4.PKE</a>, <a href="paging.htm">PKRU</a>, <a href="opc_grp.htm">RDPKRU/WRPKRU</a>, <a href="paging.htm">PxE.PK</a>, <a href="paging.htm">#PF.PK</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (UMIP)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">CR4.UMIP</a> for #GP on SGDT, SIDT, SLDT, STR, and SMSW if CPL>0</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (AVX512VBMI)</font></td>
  <td align=left><font face="Arial">AVX512VBMI</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (PREFETCHWT1)</font></td>
  <td align=left><font face="Arial">PREFETCHWT1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29</font></td>
  <td align=left><font face="Arial">ARCH_CAPABILITIES MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">28</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27 (STIBP)</font></td>
  <td align=left><font face="Arial">SPEC_CTRL.STIBP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">26 (IBRS_IBPB)</font></td>
  <td align=left><font face="Arial">SPEC_CTRL.IBRS and PRED_CMD.IBPB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">24</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">22</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">20</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">19</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">18 (PCONFIG)</font></td>
  <td align=left><font face="Arial">PCONFIG (for MK-TME)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (AVX512QFMA)</font></td>
  <td align=left><font face="Arial">V4F[N]MADD{PS,SS}</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (AVX512QVNNIW)</font></td>
  <td align=left><font face="Arial">VP4DPWSSD[S]</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0009h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0009h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0009h</font></td>
  <td align=left colspan=2><font face="Arial">get DCA parameters <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">value of PLATFORM_DCA_CAP MSR (0000_01F8h, bits 31...0)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_000Ah">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_000Ah</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_000Ah</font></td>
  <td align=left colspan=2><font face="Arial">get architectural PeMo information <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=20 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=5 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">length of EBX bit vector</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">bit width of PeMo counter(s)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">number of PeMo counters per logical processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">revision</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...7</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">branch mispredicts retired event unavailable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">branch instructions retired event unavailable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">last level cache misses event unavailable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">last level cache references event unavailable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">reference cycles event unavailable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">instructions retired event unavailable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">core cycles event unavailable</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...13</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12...5</font></td>
  <td align=left><font face="Arial">bit width of fixed-function PeMo counters (if revision &gt; 1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">number of fixed-function PeMo counters (if revision &gt; 1)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_000Bh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_000Bh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_000Bh</font></td>
  <td align=left colspan=2><font face="Arial">get topology enumeration information <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=0000_00xxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (00h=SMT)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=3><font face="Arial" width="18%">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial" size="-1">
   number of bits to shift x2APIC ID right to get unique topology ID of next level type<br>
   all logical processors with same next level ID share current level
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">number of enabled logical processors at this level</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">level type (00h=invalid, 01h=SMT, 02h=core, 03h...FFh=reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">level number (same as input)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">x2APIC ID of current logical processor</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_000Dh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_000Dh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_000Dh</font></td>
  <td align=left colspan=2><font face="Arial">get extended state enumeration <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=0000_00xxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0=main, 1=reserved, 2...62 as per <a href="crx.htm">XCR0</a>.n)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br>(main)</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">valid <a href="crx.htm">XCR0</a>.31...0 bits</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">current size (in bytes) of XSAVE/XRSTOR area (as per current XCR0)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">max. size (in bytes) of XSAVE/XRSTOR area (incl. XSAVE.HEADER)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">valid <a href="crx.htm">XCR0</a>.63...32 bits</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br>(res.)</font></td>
  <td valign=top align=center rowspan=6><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">XSAVES</a>/<a href="opc_grp.htm">XRSTORS</a> and <a href="msr.htm">XSS</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">XGETBV with ECX=1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">XSAVEC</a> and compacted form of XRSTOR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">XSAVEOPT</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">size (in bytes) in XSAVE area for <a href="crx.htm">XCR0</a> | <a href="msr.htm">XSS</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">valid <a href="msr.htm">XSS</a>.31...0 bits</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">valid <a href="msr.htm">XSS</a>.63...32 bits</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=10 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br>(sub)</font></td>
  <td valign=top align=center rowspan=2><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">
   size (in bytes) in XSAVE/XRSTOR area for <a href="crx.htm">XCR0</a>.n (n=ECX=2...62)<br>
   0 if n was invalid
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">
   offset (in bytes) in XSAVE/XRSTOR area for <a href="crx.htm">XCR0</a>.n (n=ECX=2...62)<br>
   0 if n was invalid
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...2</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">
   0 if component immediately follows previous component<br>
   1 if component is aligned to next 64 byte boundary
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">
   1 if n was valid in <a href="msr.htm">XSS</a><br>
   0 if n was invalid
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">
   reserved<br>
   0 if n was invalid
  </font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_000Fh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_000Fh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_000Fh</font></td>
  <td align=left colspan=2><font face="Arial">get platform quality of service monitoring (PQM) enumeration <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=0000_00xxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0=resources, 1...n as per EDX reported by sub-level 0)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=10 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br>(main)</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">max. range (zero-based) of RMID within this phys. processor of all types</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...2</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">L3 cache QoS monitoring</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=11 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br>(1=L3)</font></td>
  <td valign=top align=center rowspan=2><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">conversion factor from QM_CTR value to occupancy metric (bytes)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">max. range (zero-based) of RMID within this resource type</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">L3 local external bandwidth monitoring</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">L3 total external bandwidth monitoring</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">L3 occupancy monitoring</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0010h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0010h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0010h</font></td>
  <td align=left colspan=2><font face="Arial">get platform quality of service enforcement (PQE) enumeration <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=0000_00xxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0=resources, 1...n as per EBX reported by sub-level 0)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br>(main)</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">memory bandwidth allocation</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">L2 cache QoS enforcement</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">L3 cache QoS enforcement</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=13 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br>(1=L3)</font></td>
  <td valign=top align=center rowspan=3><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">length of capacity bit mask for resource n using minus-one notation</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">bit-granular map of isolation/contention of allocation units</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">code/data prioritization -- set L3_QOS_CFG MSR bit 0 to 1 to enable</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">updates of COS should be infrequent</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">highest COS number supported for resource n</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br>(2=L2)</font></td>
  <td valign=top align=center rowspan=3><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">length of capacity bit mask for resource n using minus-one notation</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">bit-granular map of isolation/contention of allocation units</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">code/data prioritization -- set L2_QOS_CFG MSR bit 0 to 1 to enable</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">1...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">highest COS number supported for resource n</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br>(3=M)</font></td>
  <td valign=top align=center rowspan=3><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">maximum MBA throttling value for resource n using minus-one notation</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">response to delay value is linear</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">1...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">highest COS number supported for resource n</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0012h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0012h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0012h</font></td>
  <td align=left colspan=2><font face="Arial">get SGX resource enumeration <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0=capabilities, 1=SECS, 2...n=EPC)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=15 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(capab.)</font></font></td>
  <td valign=top align=center rowspan=7 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...7</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">ETRACKC/ERDINFO/ELDBC/ELDUC</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">ENCLV</a> and EINCVIRTCHILD/EDECVIRTCHILD/ESETCONTEXT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4...2</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (SGX2)</font></td>
  <td align=left><font face="Arial">EAUG/EMODPR/EMODT and EACCEPT/EMODPE/EACCEPTCOPY</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (SGX1)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">ENCLS</a> and <a href="opc_grp.htm">ENCLU</a>, <a href="paging.htm">#PF.SGX</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">bit vector of supported extended features that can be written to SSA.MISC</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">maximum enclave size in 2^n bytes when not in PM64</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">maximum enclave size in 2^n bytes when in PM64</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(SECS)</font></font></td>
  <td valign=top align=center rowspan=2><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">SECS.ATTRIBUTES.31...0 that can be set with ENCLS[ECREATE]</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">SECS.ATTRIBUTES.63...32 that can be set with ENCLS[ECREATE]</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">SECS.ATTRIBUTES.95...64 that can be set with ENCLS[ECREATE]</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">SECS.ATTRIBUTES.127...96 that can be set with ENCLS[ECREATE]</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=14 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(EPC)</font></font></td>
  <td valign=top align=center rowspan=4><font face="Arial">EAX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">EPC base bits 31...12</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">11...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">3...0</font></td>
  <td align=left><font face="Arial">0000b = not valid, 0001b = level is valid, other = reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...20</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">19...0</font></td>
  <td align=left><font face="Arial">EPC base bits 51...32</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">EPC size bits 31...12</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">11...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">3...0</font></td>
  <td align=left><font face="Arial">0000b = not valid, 0001b = EPC section is protected, other = reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">31...20</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">19...0</font></td>
  <td align=left><font face="Arial">EPC size bits 51...32</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0014h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0014h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0014h</font></td>
  <td align=left colspan=2><font face="Arial">get processor trace (PT) capability enumeration <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=0000_00xxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0=capabilities, 1=details -- 1...31 as per EAX reported by sub-level 0)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=19 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(capab.)</font></font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">max sub-level</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...6</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5 (PET)</font></td>
  <td align=left><font face="Arial">power event trace, RTIT_CTL.PwrEvtEn</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4 (PTWRITE)</font></td>
  <td align=left><font face="Arial">PTWRITE, RTIT_CTL.PTWEn, RTIT.CTL.FUPonPTW</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (MTC)</font></td>
  <td align=left><font face="Arial">MTC timing packet, suppression of COFI-based packets</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (<font size="-1">IPFILT_WRSTPRSV</font>)</font></td>
  <td align=left><font face="Arial">IP filtering, TraceStop filtering, PT MSR preservation across warm reset</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (CPSB_CAM)</font></td>
  <td align=left><font face="Arial">configurable PSB, cycle-accurate mode</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">CR3 filtering</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=7><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31 (LIP)</font></td>
  <td align=left><font face="Arial">IP payloads are LIP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">output to trace transport subsystem</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (SNGLRNGOUT)</font></td>
  <td align=left><font face="Arial">single-range output scheme</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (MENTRY)</font></td>
  <td align=left><font face="Arial">ToPA tables allow multiple output entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (TOPAOUT)</font></td>
  <td align=left><font face="Arial">ToPA output</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=11 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(details)</font></font></td>
  <td valign=top align=center rowspan=4 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">bitmap of supported MTC period encodings</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2...0</font></td>
  <td align=left><font face="Arial">number of configurable address ranges for filtering</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">bitmap of supported configurable PSB frequency encodings</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">bitmap of supported cycle threshold value encodings</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0015h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0015h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0015h</font></td>
  <td align=left colspan=2><font face="Arial">get processor frequency information <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">denominator (TSC frequency = core crystal clock frequency * EBX/EAX)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">numerator (TSC frequency = core crystal clock frequency * EBX/EAX)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">core crystal clock frequency in Hz</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0016h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0016h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0016h</font></td>
  <td align=left colspan=2><font face="Arial">get processor frequency information <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=11 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=3 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">core base frequency in MHz</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">core maximum frequency in MHz</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">bus (reference) frequency in MHz</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0017h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0017h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0017h</font></td>
  <td align=left colspan=2><font face="Arial">get processor vendor attribute information <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=10 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(main)</font></font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">max sub-level</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...17</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16</font></td>
  <td align=left><font face="Arial">vendor ID uses industry standard enumeration scheme (0=no, 1=yes)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">vendor ID</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">project ID</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">stepping ID</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(1 of 3)</font></font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 1/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 2/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 3/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 4/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(2 of 3)</font></font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 5/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 6/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 7/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 8/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(3 of 3)</font></font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 9/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 10/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 11/12</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">UTF-8 encoded vendor brand string -- part 12/12</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_0018h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_0018h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_0018h</font></td>
  <td align=left colspan=2><font face="Arial">get TLB information <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0...n as per EAX reported by sub-level 0)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=20 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">
   sub-level=0: max sub-level<br>
   sub-level&gt;0: reserved
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">ways</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...11</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10...8</font></td>
  <td align=left><font face="Arial">partitioning (0: soft between logical processors sharing this TC)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">1G</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">4M</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">2M</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">4K</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">sets</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=7><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...26</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25...14</font></td>
  <td align=left><font face="Arial">max. number of addressable IDs for logical processors sharing this TC - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13...9</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">fully associative?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...5</font></td>
  <td align=left><font face="Arial">TC level (starts at 1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">TC type (00000b=invalid, 00001b=data, 00010b=code, 00011b=unified)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<a name="level_0000_001Bh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">standard level 0000_001Bh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=0000_001Bh</font></td>
  <td align=left colspan=2><font face="Arial">get PCONFIG information <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">sub-level to query (0=invalid, 1=target ID, 2...n=invalid -- see EAX bits 11...0 in output)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(invalid)</font></font></td>
  <td valign=top align=center rowspan=3 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">reserved (0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">invalid (0)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved (0)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved (0)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved (0)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output<br><font size="-2">(target ID)</font></font></td>
  <td valign=top align=center rowspan=3 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">reserved (0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">target ID (1)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">target ID 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">target ID 2</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">target ID 3</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only enabled if <a href="msr.htm">MISC_ENABLE.LCMV</a> is set to 0. This is due to a Windows NT bug.</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<a name="level_2000_0000h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=3 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Intel Xeon Phi level 2000_0000h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=2000_0000h</font></td>
  <td align=left width="76%"><font face="Arial">get maximum supported level</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">maximum supported level</font></td>
 </tr>
</table>
<br>

<a name="level_2000_0001h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Intel Xeon Phi level 2000_0001h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=2000_0001h</font></td>
  <td align=left colspan=2><font face="Arial">get processor information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=4 width="18%"><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of indicated feature</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 4 (K1OM)</font></td>
  <td align=left><font face="Arial">MVEX (62h), ZMM0...31, K0...7, transform modifiers, VSIB512, disp8*N</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 3...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<a name="level_4000_0000h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0000h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0000h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- vendor <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EBX-ECX-EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>Microsoft Hv</b></tt></td>
  <td align=left><font face="Arial">Microsoft</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>VMwareVMware</b></tt></td>
  <td align=left><font face="Arial">VMware</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>prl hyperv&nbsp;&nbsp;</b></tt></td>
  <td align=left><font face="Arial">Parallels</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<a name="level_4000_0001h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0001h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0001h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- interface <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">interface signature (e.g. 31237648h = "Hv#1")</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<a name="level_4000_0002h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0002h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0002h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- version <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=10 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">build number</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">major version</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">minor version</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">service pack</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">service branch</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...0</font></td>
  <td align=left><font face="Arial">service number</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<a name="level_4000_0003h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0003h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0003h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- features <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=49 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=15 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of features based on current privileges</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...13</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12</font></td>
  <td align=left><font face="Arial">debug MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11</font></td>
  <td align=left><font face="Arial">timer frequency MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">virtual guest idle state MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">partition reference TSC MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">access statistics pages MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">virtual system reset MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">access virtual processor index MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">hypercall MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">APIC access MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">synthetic timer MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">basic SyncIC MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">partition reference counter</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">VP runtime</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=16><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of flags specified at creation time</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...14</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13</font></td>
  <td align=left><font face="Arial">ConfigureProfiler</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12</font></td>
  <td align=left><font face="Arial">CpuManagement</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11</font></td>
  <td align=left><font face="Arial">Debugging</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">AccessStats</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">ConnectPort</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">CreatePort</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">SignalEvents</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">PostMessages</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">AdjustMessageBuffers</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">AccessMemoryPool</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">AccessPartitionId</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">CreatePartition</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of power management information</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">HPET is required to enter C3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3..0</font></td>
  <td align=left><font face="Arial">maximum processor power state (0=C0, 1=C1, 2=C2, 3=C3)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=14><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of miscellaneous available features</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11</font></td>
  <td align=left><font face="Arial">debug MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">guest crash MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">inject synthetic MCs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">determine timer frequencies</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">query NUMA distances</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">hypervisior sleep state</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">virtual guest idle state</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">hypercall input parameter block via XMM</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">physical CPU dynamic partitioning events</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">performance monitor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">guest debugging</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">MWAIT</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<a name="level_4000_0004h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0004h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0004h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- recommendations <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=18 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=12 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">deprecate AutoEOI</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">x2APIC MSRs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">interrupt remapping</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">DMA remapping</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">relaxed timing -- disable watchdogs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">MSR for system reset</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">MSRs for APIC EOI/ICR/TPR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">hypercall for remote TLB flush</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">hypercall for local TLB flush</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">hypercall for address space switch</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">recommended spinlock failure retries (FFFF_FFFFh = -1 = never)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<a name="level_4000_0005h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0005h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0005h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- limits <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">maximum supported virtual processors</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">maximum supported logical processors</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">maximum supported physical interrupt vectors for remapping</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<a name="level_4000_0006h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">hypervisor level 4000_0006h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=4000_0006h</font></td>
  <td align=left colspan=2><font face="Arial">get hypervisor information -- hardware features detected and in use<sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=15 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=9 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...7</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">memory patrol scrubber</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">interrupt remapping</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">DMA remapping</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">second level address translation</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">architectural performance counters</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">MSR bitmaps</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">APIC overlay assist</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">reserved for future AMD-specific features</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center valign=top bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only available if implemented by the hypervisor.</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<a name="level_8000_0000h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0000h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0000h</font></td>
  <td align=left colspan=2><font face="Arial">get maximum supported extended level and vendor ID string</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">maximum supported extended level</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">EBX-EDX-ECX</font></td>
  <td align=left colspan=2><font face="Arial">vendor ID string</font></td>
 </tr>
 <tr>
  <td align=center width="18%"><tt><b>AuthenticAMD</b></tt></td>
  <td width="58%" align=left><font face="Arial">AMD processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">reserved</font></td>
  <td align=left><font face="Arial">Cyrix processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">reserved</font></td>
  <td align=left><font face="Arial">Centaur processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">reserved</font></td>
  <td align=left><font face="Arial">Intel processor</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>TransmetaCPU</b></tt></td>
  <td align=left><font face="Arial">Transmeta processor</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">reserved</font></td>
  <td align=left><font face="Arial">National Semiconductor processor (GX1, GXLV, GXm)</font></td>
 </tr>
 <tr>
  <td align=center><tt><b>Geode by NSC</b></tt></td>
  <td align=left><font face="Arial">National Semiconductor processor (GX2)</font></td>
 </tr>
</table>
<br>

<a name="level_8000_0001h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=6 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0001h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8000_0001h</font></td>
  <td align=left colspan=4><font face="Arial">get processor family/model/stepping and features flags</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td width=50 valign=top align=center rowspan=553 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td width=160 valign=top align=center rowspan=106><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left colspan=4><font face="Arial">processor family/model/stepping</font></td>
 </tr>
 <tr>
  <td width=160 valign=top align=center rowspan=9 bgcolor="#004080"><font color="#FFFFFF" face="Arial">
   extended family<br>
   (add)
  </font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The extended processor family is encoded in bits 27...20.</font></td>
 </tr>
 <tr>
  <td width=160 align=center rowspan=8></td>
  <td width=50 valign=top align=center><font face="Arial">00<font color="#808080">+F</font></font></td>
  <td width=296 align=left><font face="Arial">
   AMD K8 (Fam 08h)<br>
   Transmeta Efficeon
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">01<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD K8L (Fam 10h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">02<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD K8 (Fam 11h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">03<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD K8L (Fam 12h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">05<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD BC (Fam 14h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">06<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD BD (Fam 15h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">07<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD JG (Fam 16h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">08<font color="#808080">+F</font></font></td>
  <td align=left><font face="Arial">AMD ZN (Fam 17h)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=19 bgcolor="#004080"><font color="#FFFFFF" face="Arial">
   extended model<br>
   (concat)
  </font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The extended processor model is encoded in bits 19...16.</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">AMD K8</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">130 nm Rev C</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">90 nm Rev D</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">90 nm Rev E</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">90 nm Rev F</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">90 nm Rev F</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">65 nm Rev G</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">65 nm Rev G</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">C</font></td>
  <td align=left><font face="Arial">90 nm Rev F (in Fr3)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">AMD Fam 15h</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">OR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">TN/RL</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">KV/GV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">CZ/BR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">ST</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">AMD Fam 16h</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">KB/BV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">ML</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">NL</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">AMD Fam 17h</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">ZP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">RV</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5 bgcolor="#004080"><font color="#FFFFFF" face="Arial">family</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The family is encoded in bits 11...8.</font></td>
 </tr>
 <tr>
  <td align=center rowspan=4></td>
  <td valign=top align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">
   AMD K5<br>
   Geode<br>
   Centaur C2 and C3<br>
   Transmeta Crusoe
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">
   AMD K6<br>
   VIA C3
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">AMD K7</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">F</font></td>
  <td align=left><font face="Arial">refer to extended family</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=70 bgcolor="#004080"><font color="#FFFFFF" face="Arial">model</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The model is encoded in bits 7...4.</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">AMD K5</font></td>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">5k86 (PR120 or PR133)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">5k86 (PR166)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">5k86 (PR200)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">AMD K6</font></td>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">K6 (0.30 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">K6 (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">K6-2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">K6-III</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">D</font></td>
  <td align=left><font face="Arial">K6-2+ or K6-III+ (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">AMD K7</font></td>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Athlon (0.25 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Athlon (0.18 &micro;m)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">Duron (SF core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">Athlon (TB core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">Athlon (PM core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">Duron (MG core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">Athlon (TH/AP core)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">Athlon (BT core)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=9><font face="Arial">AMD K8 (Fam 08h)</font></td>
  <td align=center><font face="Arial">xx00b</font></td>
  <td align=left><font face="Arial">Socket 754 or Socket S1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">xx01b</font></td>
  <td align=left><font face="Arial">Socket 940 or Socket F1207</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">xx10b</font></td>
  <td align=left><font face="Arial">if Rev CG, then see K8 erratum #108</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">xx11b</font></td>
  <td align=left><font face="Arial">Socket 939 or Socket AM2 or ASB1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">01xxb</font></td>
  <td align=left><font face="Arial">SH (SC 1024 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11xxb</font></td>
  <td align=left><font face="Arial">DH (SC 512 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10xxb</font></td>
  <td align=left><font face="Arial">CH (SC 256 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">00xxb</font></td>
  <td align=left><font face="Arial">JH (DC 1024 KB)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10xxb</font></td>
  <td align=left><font face="Arial">BH (DC 512 KB)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">AMD K8L (Fam 10h)</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Rev A DR (0/1/2=A0/A1/A2)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Rev B DR (0/1/A/2/3=B0/B1/BA/B2/B3)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4/5/6</font></td>
  <td align=left><font face="Arial">Rev C RB/BL/DA (0/1/2/3=C0/C1/C2/C3)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8/9</font></td>
  <td align=left><font face="Arial">Rev D HY SCM/MCM (0/1=D0/D1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">A</font></td>
  <td align=left><font face="Arial">Rev E PH (0=E0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K8 (Fam 11h)</font></td>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">Rev B LG (1=B1)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">AMD K8L (Fam 12h)</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">Rev A LN1 (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Rev B LN1 (0=B0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Rev B LN2 (0=B0)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">AMD BC (Fam 14h)</font></td>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">Rev B ON (0=B0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">Rev C ON (0=C0)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=10><font face="Arial">AMD BD (Fam 15h)</font></td>
  <td align=center><font face="Arial"><font color="#808080">0</font>0</font></td>
  <td align=left><font face="Arial">Rev A OR (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">0</font>1</font></td>
  <td align=left><font face="Arial">Rev B OR (0/1/2=B0/B1/B2)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">0</font>2</font></td>
  <td align=left><font face="Arial">Rev C OR (0=C0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>0</font></td>
  <td align=left><font face="Arial">Rev A TN (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>3</font></td>
  <td align=left><font face="Arial">Rev A RL (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>0</font></td>
  <td align=left><font face="Arial">Rev A KV (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>8</font></td>
  <td align=left><font face="Arial">Rev A GV (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">6</font>0</font></td>
  <td align=left><font face="Arial">Rev A CZ (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center valign=top><font face="Arial"><font color="#808080">6</font>5</font></td>
  <td align=left><font face="Arial">
   OSVW.ID5=0: Rev A CZ DDR4 (1=A1)<br>
   OSVW.ID5=1: Rev A BR (1=A1)
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">7</font>0</font></td>
  <td align=left><font face="Arial">Rev A ST (0=A0)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=4><font face="Arial">AMD JG (Fam 16h)</font></td>
  <td align=center><font face="Arial"><font color="#808080">0</font>0</font></td>
  <td align=left><font face="Arial">Rev A KB (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">0</font>4</font></td>
  <td align=left><font face="Arial">Rev A BV (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">3</font>0</font></td>
  <td align=left><font face="Arial">Rev A ML (0/1=A0/A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">6</font>0</font></td>
  <td align=left><font face="Arial">Rev A NL (1=A1)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">AMD ZN (Fam 17h)</font></td>
  <td align=center><font face="Arial"><font color="#808080">0</font>0</font></td>
  <td align=left><font face="Arial">Rev A ZP (1=A1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial"><font color="#808080">1</font>0</font></td>
  <td align=left><font face="Arial">Rev A RV (1=A1)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">Geode</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">GX1, GXLV, GXm</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">GX2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">LX</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">Centaur</font></td>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">C2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">C3</font></td>
 </tr>
 <tr>
  <td rowspan=8 valign=top align=center><font face="Arial">VIA C3</font></td>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">Cyrix M2 core</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">WinChip C5A core</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">WinChip C5B core (if stepping = 0...7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">WinChip C5C core (if stepping = 8...F)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">WinChip C5N core (if stepping = 0...7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">WinChip C5XL core (if stepping = 0...7)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">WinChip C5P core (if stepping = 8...F)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">WinChip C5J core</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Crusoe</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">TM3x00 and TM5x00</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">Transmeta Efficeon</font></td>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">TM8000 (130 nm)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">TM8000 (90 nm CMS 6.0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">TM8000 (90 nm CMS 6.1+)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">stepping</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The stepping is encoded in bits 3...0.</font></td>
 </tr>
 <tr>
  <td align=left colspan=3><font face="Arial">The stepping values are processor-specific.</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=381><font face="Arial">EBX=x000_xxxxh</font></td>
  <td align=center valign=top rowspan=36 bgcolor="#004080"><font color="#FFFFFF" face="Arial">package type</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The package type is encoded in bits 31...28.</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=7><font face="Arial">AMD K8L (Fam 10h)</font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket F</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket AM</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket S1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0011b</font></td>
  <td align=left><font face="Arial">Socket G34</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0100b</font></td>
  <td align=left><font face="Arial">Socket ASB2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0101b</font></td>
  <td align=left><font face="Arial">Socket C32</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">other</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">AMD K8L (Fam 12h)</font></td>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket FS1 (&micro;PGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket FM1 (PGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD BC (Fam 14h)</font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FT1 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">
   AMD BD (Fam 15h)<br>
   <font size="-1">extended model 0</font>
  </font></td>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket AM3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0011b</font></td>
  <td align=left><font face="Arial">Socket G34</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0101b</font></td>
  <td align=left><font face="Arial">Socket C32</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">
   AMD BD (Fam 15h)<br>
   <font size="-1">extended model 1</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FP2 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket FS1r2 (&micro;PGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket FM2 (PGA)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">
   AMD BD (Fam 15h)<br>
   <font size="-1">extended model 3</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FP3 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket FM2r2 (&micro;PGA)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">
   AMD BD (Fam 15h)<br>
   <font size="-1">extended model 6</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FP4 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket AM4 (&micro;PGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0011b</font></td>
  <td align=left><font face="Arial">Socket FM2r2 (&micro;PGA)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">
   AMD BD (Fam 15h)<br>
   <font size="-1">extended model 7</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FP4 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket AM4 (&micro;PGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0100b</font></td>
  <td align=left><font face="Arial">Socket FT4 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">
   AMD JG (Fam 16h)
   <font size="-1">extended model 0</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FT3 (BGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket FS1b</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">
   AMD JG (Fam 16h)
   <font size="-1">extended model 3</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FT3b (BGA)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0011b</font></td>
  <td align=left><font face="Arial">Socket FP4</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=5><font face="Arial">
   AMD ZP (Fam 17h)
   <font size="-1">extended model 0</font>
  </font></td>
  <td align=center><font face="Arial">0001b</font></td>
  <td align=left><font face="Arial">Socket SP4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket AM4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0011b</font></td>
  <td align=left><font face="Arial">Socket SP4r2</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0100b</font></td>
  <td align=left><font face="Arial">Socket SP3</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0111b</font></td>
  <td align=left><font face="Arial">Socket SP3r2</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2><font face="Arial">
   AMD RV (Fam 17h)
   <font size="-1">extended model 1</font>
  </font></td>
  <td align=center><font face="Arial">0000b</font></td>
  <td align=left><font face="Arial">Socket FP5</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0010b</font></td>
  <td align=left><font face="Arial">Socket AM4</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=345 bgcolor="#004080"><font color="#FFFFFF" face="Arial">brand ID</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The brand ID is encoded in bits 15...0.</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K8 DDR1</font></td>
  <td colspan=2 align=left><font face="Arial">
   ID = bits 15...6 = (value >> 6) & 3FFh<br>
   NN = bits 5...0 = value & 3Fh<br>
   &nbsp;<br>
   for NN=1...63: <b>XX</b> = 22 + NN<br>
   for NN=1...30: <b>YY</b> = 38 + (2 * NN)<br>
   for NN=1...63: <b>ZZ</b> = 24 + NN<br>
   for NN=1...63: <b>TT</b> = 24 + NN<br>
   for NN=1...11: <b>RR</b> = 45 + (5 * NN)<br>
   for NN=1...31: <b>EE</b> = 9 + NN
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">00h</font></td>
  <td colspan=2 align=left><font face="Arial">engineering sample</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">04h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>XX</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">05h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>XX</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">06h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 FX-<b>ZZ</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">08h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>XX</b>00+ mobile</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">09h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>XX</b>00+ mobile, low power</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion 64 ML-<b>XX</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Bh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion 64 MT-<b>XX</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 1<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Dh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 1<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Eh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 1<b>YY</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0Fh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 1<b>YY</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 2<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 2<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 2<b>YY</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 2<b>YY</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 8<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 8<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 8<b>YY</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 8<b>YY</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">18h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>EE</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1Dh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon XP-M <b>XX</b>00+ mobile, 32-bit</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1Eh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon XP-M <b>XX</b>00+ mobile, 32-bit, low power</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">20h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon XP <b>XX</b>00+, 32-bit</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+ mobile, 32-bit</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+ mobile, 32-bit, low power</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">22h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+, 32-bit</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">26h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+, 64-bit</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">24h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 FX-<b>ZZ</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b> SE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b> SE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Bh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b> SE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Dh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Eh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2Fh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">32h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">33h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">34h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">35h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">36h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">37h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">38h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">39h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Bh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 1<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 2<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3Dh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 8<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">other</font></td>
  <td colspan=2 align=left><font face="Arial">unknown</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K8 DDR2</font></td>
  <td colspan=2 align=left><font face="Arial">
   S = socket (see CPUID model bits 1...0)<br>
   CC = core count - 1 (see NB capabilities register)<br>
   ID = bits 13...9<br>
   PL = bits 8...6 and 14<br>
   NN = bits 15 and 5...0<br>
   &nbsp;<br>
   <b>RR</b> = -1 + NN<sup>*</sup><br>
   <b>PP</b> = 26 + NN<br>
   <b>TT</b> = 15 + (CC * 10) + NN<br>
   <b>ZZ</b> = 57 + NN<sup>**</sup><br>
   <b>YY</b> = 29 + NN<br>
   <font size="-2">
    * 000001b...000010b/100010b...111111b = 1...2/34...63 are reserved<br>
    ** 100010b...111111b = 34...63 are reserved
   </font>
  </font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=any CC=? ID=00h PL=0h</font></td>
  <td colspan=2 align=left><font face="Arial">engineering sample</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=01h PL=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron LE-1<b>RR</b>0</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=02h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon LE-1<b>ZZ</b>0</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=03h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 1<b>ZZ</b>0B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=04h PL=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=04h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=04h PL=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=04h PL=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=04h PL=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=04h PL=8h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=05h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>RR</b>50p</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=06h PL=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=06h PL=8h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=ASB1 CC=0 ID=07h PL=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>0U</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=ASB1 CC=0 ID=07h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>0U</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=08h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon <b>TT</b>50e</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=0 ID=08h PL=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon <b>TT</b>50e</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=ASB1 CC=0 ID=09h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon Neo MV-<b>TT</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=ASB1 CC=0 ID=0Ch PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron 2<b>RR</b>U</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=01h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 12<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=01h PL=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 12<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=01h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 12<b>RR</b> SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=03h PL=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon X2 BE-2<b>TT</b>0</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=04h PL=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=04h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=04h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=04h PL=8h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=04h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=05h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 FX-<b>ZZ</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=06h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>RR</b>00</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=07h PL=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon <b>TT</b>50e</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=07h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon <b>TT</b>00B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=07h PL=7h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon <b>TT</b>00B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=08h PL=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon <b>TT</b>50B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=09h PL=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon X2 <b>TT</b>50e</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=0Ah PL=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon Neo X2 <b>TT</b>50e</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=AM2 CC=1 ID=0Ah PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon Neo X2 <b>TT</b>50e</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=ASB1 CC=1 ID=0Bh PL=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion Neo X2 L6<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=ASB1 CC=1 ID=0Ch PL=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon Neo X2 L3<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=01h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=02h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion 64 MK-<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=03h PL=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+ mobile</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=03h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>PP</b>00+ mobile</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=03h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>PP</b>00+ mobile</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=04h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=06h PL=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon TF-<b>TT</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=06h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon TF-<b>TT</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=06h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon TF-<b>TT</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=0 ID=07h PL=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon L1<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=01h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron TJ-<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=02h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion 64 X2 TL-<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=03h PL=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 TK-<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=03h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 TK-<b>YY</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=05h PL=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 X2 <b>TT</b>00+</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=06h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon X2 L3<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=S1 CC=1 ID=07h PL=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion X2 L5<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=0 ID=01h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron 22<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=00h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 12<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=00h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 12<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=01h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 22<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=01h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 22<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=01h PL=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 22<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=01h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 22<b>RR</b> SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=04h PL=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 82<b>RR</b> EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=04h PL=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 82<b>RR</b> HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=04h PL=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 82<b>RR</b></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=04h PL=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron DC 82<b>RR</b> SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">S=F1207 CC=1 ID=06h PL=Eh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon 64 FX-<b>ZZ</b> (Fr3)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K8L (Fam 10h)</font></td>
  <td colspan=2 align=left><font face="Arial">
   PT = package type (se EBX bits 31...28)<br>
   NC = number of cores (see level 8000_0008h)<br>
   &nbsp;<br>
   PG = bit 15<br>
   S1 = bits 14...11<br>
   M = bits 10...4<br>
   S2 = bits 3...0
  </font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=3 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">QC AMD Opteron Processor 83</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=3 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">QC AMD Opteron Processor 23</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=5 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">6C AMD Opteron Processor 84</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=5 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">6C AMD Opteron Processor 24</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">Embedded AMD Opteron Processor_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=5 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">Embedded AMD Opteron Processor_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=3 <i>S2</i>=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=3 <i>S2</i>=Bh</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=3 <i>S2</i>=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=5 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=5 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=5 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">GF HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">HF HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">VS</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">QS HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=5h</font></td>
  <td colspan=2 align=left><font face="Arial">NP HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=6h</font></td>
  <td colspan=2 align=left><font face="Arial">KH HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=3 <i>S2</i>=7h</font></td>
  <td colspan=2 align=left><font face="Arial">KS EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=5 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">QS</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=1 NC=5 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">KS HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=0 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron 1</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=0 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II 1</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X2 2</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X2 B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X2_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=7h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X2 5</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X2_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=Bh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X2 B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron X2 1</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X3 B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X3_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 S1=7h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X3 4</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 S1=8h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X3 7</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 S1=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X3_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">QC AMD Opteron Processor 13</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4 9</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4 8</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=7h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4 B</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=8h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X4 6</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 S1=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X4_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=5 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X6 1</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II XLT V</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=1 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II XL V</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=3 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II XLT Q</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=3 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4 9</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=3 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4 8</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=3 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X4 6</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=0 <i>S2</i>=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=0 <i>S2</i>=Bh</font></td>
  <td colspan=2 align=left><font face="Arial">u Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">50 DC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=6h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=7h</font></td>
  <td colspan=2 align=left><font face="Arial">e Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=9h</font></td>
  <td colspan=2 align=left><font face="Arial">0 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">0e Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=Bh</font></td>
  <td colspan=2 align=left><font face="Arial">u Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">00 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">00e 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">00B 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">50 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">50e 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=5h</font></td>
  <td colspan=2 align=left><font face="Arial">50B 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=6h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=7h</font></td>
  <td colspan=2 align=left><font face="Arial">e Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=9h</font></td>
  <td colspan=2 align=left><font face="Arial">0e Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=2 <i>S2</i>=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">0 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">00 QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">00e QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">00B QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">50 QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">50e QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=5h</font></td>
  <td colspan=2 align=left><font face="Arial">50B QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=6h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=7h</font></td>
  <td colspan=2 align=left><font face="Arial">e Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=9h</font></td>
  <td colspan=2 align=left><font face="Arial">0e Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=Eh</font></td>
  <td colspan=2 align=left><font face="Arial">0 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=5 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">5T Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=5 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">0T Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=1 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">L Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=1 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">L Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=1 NC=3 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">T Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=0 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron M1</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=0 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD V</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion II Ultra DC Mobile M6</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion II DC Mobile M5</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II DC M3</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion II P</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II P</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=7h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion II N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=8h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=9h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II P</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=2 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II P</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=2 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=2 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II P</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II X</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Phenom II N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=0 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">0 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">0 DC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=2 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">0 3C Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">0 QC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=7 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron Processor 61</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=B S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron Processor 61</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=1 NC=7 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">Embedded AMD Opteron Processor_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=7 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=7 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=B <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=B <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;SE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=1 NC=7 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">QS</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=3 PG=1 NC=7 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">KS</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=0 S1=1b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II Neo K</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=0 S1=2b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD V</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=0 S1=3b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II Neo R</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 S1=1b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion II Neo K</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 S1=2b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II Neo K</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 S1=3b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD V</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 S1=4b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion II Neo N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 S1=5b</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II Neo N</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=0 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">5 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=0 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">L Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">5 DC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">L DC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=1 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">H DC Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=4 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=3 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron Processor 41</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=5 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Opteron Processor 41</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=3 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">Embedded AMD Opteron Processor_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=5 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">Embedded AMD Opteron Processor_</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=3 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=5 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=5 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">QS HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=3 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">LE HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=3 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">CL EE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=5 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">KX HE</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=5 PG=1 NC=5 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">GL EE</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K8L (Fam 11h)</font></td>
  <td colspan=2 align=left><font face="Arial">
   PT = package type (se EBX bits 31...28)<br>
   NC = number of cores (see level 8000_0008h)<br>
   &nbsp;<br>
   PG = bit 15<br>
   S1 = bits 14...11<br>
   M = bits 10...4<br>
   S2 = bits 3...0
  </font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=0 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron SI-</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=0 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon QI-</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=0h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion X2 Ultra Dual-Core Mobile ZM-</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Turion X2 Dual-Core Mobile RM-</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon X2 Dual-Core QL-</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron X2 Dual-Core NI-</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=0 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 <i>S2</i>=0h</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K8L (Fam 12h)</font></td>
  <td colspan=2 align=left><font face="Arial">
   PT = package type (se EBX bits 31...28)<br>
   NC = number of cores (see level 8000_0008h)<br>
   &nbsp;<br>
   PG = bit 15<br>
   S1 = bits 14...11<br>
   M = bits 10...4<br>
   S2 = bits 3...0
  </font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A4-33</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E2-30</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=4 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A8-35</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=4 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A6-34</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">M APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=1 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">MX APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">M APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=3 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">MX APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=1 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A4-33</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E2-32</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X2 2</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A4-34</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Sempron X2 1</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=2 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A6-35</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A8-38</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD A6-36</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=Dh</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Athlon II X4 6</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=1 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Dual-Core Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=2 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=3 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Quad-Core Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=2 PG=0 NC=x S1=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD BC (Fam 14h)</font></td>
  <td colspan=2 align=left><font face="Arial">
   PT = package type (se EBX bits 31...28)<br>
   NC = number of cores (see level 8000_0008h)<br>
   &nbsp;<br>
   PG = bit 15<br>
   S1 = bits 14...11<br>
   M = bits 10...4<br>
   S2 = bits 3...0
  </font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD C- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD G-T- <font color="#808080">(embedded)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=1h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD C- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=2h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=3h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD Z- <font color="#808080">(tablet)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=4h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD G-T- <font color="#808080">(embedded)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=5h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E1-1- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=6h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E2-1- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 S1=7h</font></td>
  <td colspan=2 align=left><font face="Arial">AMD E2-2- <font color="#808080">(client)</font></font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">0 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">5 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">0x Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=5h</font></td>
  <td colspan=2 align=left><font face="Arial">5x Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=6h</font></td>
  <td colspan=2 align=left><font face="Arial">x Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=7h</font></td>
  <td colspan=2 align=left><font face="Arial">L Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=8h</font></td>
  <td colspan=2 align=left><font face="Arial">N Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=9h</font></td>
  <td colspan=2 align=left><font face="Arial">R Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">0 APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=Bh</font></td>
  <td colspan=2 align=left><font face="Arial">5 APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=0 <i>S2</i>=Dh</font></td>
  <td colspan=2 align=left><font face="Arial">0D APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=1h</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=2h</font></td>
  <td colspan=2 align=left><font face="Arial">0 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=3h</font></td>
  <td colspan=2 align=left><font face="Arial">5 Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=4h</font></td>
  <td colspan=2 align=left><font face="Arial">0x Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=5h</font></td>
  <td colspan=2 align=left><font face="Arial">5x Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=6h</font></td>
  <td colspan=2 align=left><font face="Arial">x Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=7h</font></td>
  <td colspan=2 align=left><font face="Arial">L Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=8h</font></td>
  <td colspan=2 align=left><font face="Arial">N Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=9h</font></td>
  <td colspan=2 align=left><font face="Arial">0 APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=Ah</font></td>
  <td colspan=2 align=left><font face="Arial">5 APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=Bh</font></td>
  <td colspan=2 align=left><font face="Arial">&nbsp;APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=Ch</font></td>
  <td colspan=2 align=left><font face="Arial">E Processor</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=1 <i>S2</i>=Dh</font></td>
  <td colspan=2 align=left><font face="Arial">0D APU with Radeon HD Graphics</font></td>
 </tr>
 <tr>
  <td align=center><font size="-2" face="Arial">PT=0 PG=0 NC=x <i>S2</i>=Fh</font></td>
  <td colspan=2 align=left><font face="Arial">(empty)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of indicated feature</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 31</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 30</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 29 (MONX)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">MONITORX/MWAITX</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 28 (PCX_L2I / L3)</font></td>
  <td align=left colspan=3><font face="Arial">
   L2I perf counter extensions (MSRs C001_023[0...7]h) (Fam 15h/16h)<br>
   L3 perf counter extensions (MSRs C001_023[0...B]h) (Fam 17h)
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 27 (PERFTSC)</font></td>
  <td align=left colspan=3><font face="Arial">performance TSC (MSR C001_0280h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 26 (DBX)</font></td>
  <td align=left colspan=3><font face="Arial">data breakpoint extensions (MSRs C001_1027h and C001_10[19...1B]h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 25</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 24 (PCX_NB)</font></td>
  <td align=left colspan=3><font face="Arial">NB perf counter extensions (MSRs C001_024[0...7]h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 23 (PCX_CORE)</font></td>
  <td align=left colspan=3><font face="Arial">core perf counter extensions (MSRs C001_020[0...B]h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 22 (TOPX)</font></td>
  <td align=left colspan=3><font face="Arial">topology extensions: extended levels 8000_001Dh and 8000_001Eh</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 21 (TBM)</font></td>
  <td align=left colspan=3><font face="Arial">TBM</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 20</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 19 (NODEID)</font></td>
  <td align=left colspan=3><font face="Arial">node ID: MSR C001_100Ch</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 18</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 17 (TCE)</font></td>
  <td align=left colspan=3><font face="Arial">translation cache extension, <a href="msr.htm">EFER.TCE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 16 (FMA4)</font></td>
  <td align=left colspan=3><font face="Arial">FMA4</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 15 (LWP)</font></td>
  <td align=left colspan=3><font face="Arial">LWP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 14</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 13 (WDT)</font></td>
  <td align=left colspan=3><font face="Arial">watchdog timer</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 12 (SKINIT)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">SKINIT</a>, <a href="opc_grp.htm">STGI</a>, DEV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 11 (XOP)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_xop.htm">XOP</a> (was also used going to be used for <a href="opc_3a.htm">SSE5A</a>)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 10 (IBS)</font></td>
  <td align=left colspan=3><font face="Arial">instruction based sampling</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 9 (OSVW)</font></td>
  <td align=left colspan=3><font face="Arial">OS-visible workaround</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 8 (3DNow!P)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_k3d.htm">PREFETCH</a> and <a href="opc_k3d.htm">PREFETCHW</a> (K8 Rev G and K8L+)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 7 (MSSE)</font></td>
  <td align=left colspan=3><font face="Arial">misaligned SSE, <a href="fp_new.htm">MXCSR.MM</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 6 (SSE4A)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">SSE4A</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 5 (LZCNT)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">LZCNT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 4 (CR8D)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">MOV from/to CR8D</a> by means of LOCK-prefixed MOV from/to CR0</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 3 (EAS)</font></td>
  <td align=left colspan=3><font face="Arial">extended APIC space (APIC_VER.EAS, EXT_APIC_FEAT, etc.)</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">bit 2 (SVM)</font></td>
  <td align=left colspan=3><font face="Arial">
   <a href="msr.htm">EFER.SVME</a><br>
   <a href="opc_grp.htm">VMRUN, VMMCALL, VMLOAD and VMSAVE, STGI and CLGI,<br>
   SKINIT, INVLPGA<a>
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 1 (CMP)</font></td>
  <td align=left colspan=3><font face="Arial">HTT=1 indicates HTT (0) or CMP (1)</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#B0D0D0"><font face="Arial">bit 0 (AHF64)</font></td>
  <td align=left colspan=3 bgcolor="#B0D0D0"><font face="Arial"><a href="opc_1.htm">LAHF</a> and <a href="opc_1.htm">SAHF</a> in <a href="mode.htm">PM64</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=33><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of indicated feature</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 31 (3DNow!)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_k3d.htm">3DNow!</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 30 (3DNow!+)</font></td>
  <td align=left colspan=3><font face="Arial">extended <a href="opc_k3d.htm">3DNow!</a></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#B0D0D0"><font face="Arial">bit 29 (LM)</font></td>
  <td align=left colspan=3 bgcolor="#B0D0D0"><font face="Arial">AMD64/EM64T, Long Mode</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 28</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 27 (TSCP)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">TSC</a>, <a href="msr.htm">TSC_AUX</a>, <a href="opc_grp.htm">RDTSCP</a>, <a href="crx.htm">CR4.TSD</a></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#B0D0D0"><font face="Arial">bit 26 (PG1G)</font></td>
  <td align=left colspan=3 bgcolor="#B0D0D0"><font face="Arial"><a href="paging.htm">PML3E.PS</a></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#B0D0D0"><font face="Arial">bit 25 (FFXSR)</font></td>
  <td align=left colspan=3 bgcolor="#B0D0D0"><font face="Arial"><a href="msr.htm">EFER.FFXSR</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">
   bit 24 (MMX+)<br>
   bit 24 (FXSR)
  </font></td>
  <td align=left colspan=3><font face="Arial">
   Cyrix specific: <a href="opc_2.htm">extended MMX</a><br>
   AMD K7: <a href="opc_grp.htm">FXSAVE/FXRSTOR</a>, <a href="crx.htm">CR4.OSFXSR</a>
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 23 (MMX)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">MMX</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 22 (MMX+)</font></td>
  <td align=left colspan=3><font face="Arial">AMD specific: <a href="opc_2.htm">MMX-SSE and SSE-MEM</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 21</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 20 (NX)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">EFER.NXE</a>, <a href="paging.htm">P?E.NX</a>, <a href="paging.htm">#PF(1xxxx)</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 19 (MP)</font></td>
  <td align=left colspan=3><font face="Arial">MP-capable <sup>#3</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 18</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 17 (PSE36)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">4 MB PDE bits 16...13</a>, <a href="crx.htm">CR4.PSE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">
   bit 16 (FCMOV)<br>
   bit 16 (PAT)
  </font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_fpu.htm">
   FCMOVcc/F(U)COMI(P)</a> (implies FPU=1)<br>
   AMD K7: <a href="msr.htm">PAT MSR</a>, <a href="paging.htm">PDE/PTE.PAT</a>
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 15 (CMOV)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">CMOVcc</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 14 (MCA)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MCG_*/MCn_* MSRs</a>, <a href="crx.htm">CR4.MCE</a>, <a href="except.htm">#MC</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 13 (PGE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">PDE/PTE.G</a>, <a href="crx.htm">CR4.PGE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 12 (MTRR)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MTRR* MSRs</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 11 (SEP)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_2.htm">SYSCALL/SYSRET</a>, <a href="msr.htm">EFER/STAR MSRs</a> <sup>#1</sup></a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 10</font></td>
  <td align=left colspan=3><font face="Arial">reserved <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 9 (APIC)</font></td>
  <td align=left colspan=3><font face="Arial">APIC <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 8 (CX8)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_grp.htm">CMPXCHG8B</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 7 (MCE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MCAR/MCTR MSRs</a>, <a href="crx.htm">CR4.MCE</a>, <a href="except.htm">#MC</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 6 (PAE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">64-bit PDPTE/PDE/PTEs</a>, <a href="crx.htm">CR4.PAE</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 5 (MSR)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">MSRs</a>, <a href="opc_2.htm">RDMSR/WRMSR</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 4 (TSC)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="msr.htm">TSC</a>, <a href="opc_2.htm">RDTSC</a>, <a href="crx.htm">CR4.TSD</a> (doesn't imply MSR=1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 3 (PSE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="paging.htm">PDE.PS</a>, <a href="paging.htm">PDE/PTE.res</a>, <a href="crx.htm">CR4.PSE</a>, <a href="except.htm">#PF(1xxxb)</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 2 (DE)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.DE</a>, <a href="drx.htm">DR7.RW=10b</a>, <a href="except.htm">#UD</a> on MOV from/to DR4/5</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 1 (VME)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="crx.htm">CR4.VME/PVI</a>, <a href="flags.htm">EFLAGS.VIP/VIF</a>, <a href="tss.htm">TSS32.IRB</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 0 (FPU)</font></td>
  <td align=left colspan=3><font face="Arial"><a href="opc_fpu.htm">FPU</a></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=5 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=5><font face="Arial">The AMD K6 processor, model 6, uses bit 10 to indicate SEP. Beginning with model 7, bit 11 is used instead.</font></td>
 </tr>
 <tr>
  <td align=left colspan=5 bgcolor="#B0D0D0"><font face="Arial">Intel processors only report SEP when CPUID is executed in PM64.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=5><font face="Arial">If the APIC has been disabled, then the APIC feature flag will read as 0.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#3</font></td>
  <td align=left colspan=5><font face="Arial">AMD K7 processors prior to CPUID=0662h may report 0 even if they are MP-capable.</font></td>
 </tr>
</table>
<br>

<a name="level_8000_0002h">
<a name="level_8000_0003h">
<a name="level_8000_0004h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended levels 8000_0002h, 8000_0003h, and 8000_0004h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=3 align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0002h</font></td>
  <td align=left colspan=2><font face="Arial">get processor name string (part 1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EAX=8000_0003h</font></td>
  <td align=left colspan=2><font face="Arial">get processor name string (part 2)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EAX=8000_0004h</font></td>
  <td align=left colspan=2><font face="Arial">get processor name string (part 3)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=21 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=21><font face="Arial">
   EAX<br>
   EBX<br>
   ECX<br>
   EDX<br>
  </font></td>
  <td align=left colspan=2><font face="Arial">processor name string <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center width="18%"><font face="Arial">AMD K5</font></td>
  <td width="58%" align=left><tt><b>AMD-K5(tm) Processor</b></tt></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K6</font></td>
  <td align=left><tt><b>AMD-K6tm w/ multimedia extensions</b></tt></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K6-2</font></td>
  <td align=left>
   <tt><b>AMD-K6(tm) 3D processor</b></tt><br>
   <tt><b>AMD-K6(tm)-2 Processor</b></tt><br>
  </td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K6-III</font></td>
  <td align=left>
   <tt><b>AMD-K6(tm) 3D+ Processor</b></tt><br>
   <tt><b>AMD-K6(tm)-III Processor</b></tt><br>
  </td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K6-2+</font></td>
  <td align=left><tt><b>AMD-K6(tm)-III Processor</b></tt> (?)</td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K6-III+</font></td>
  <td align=left><tt><b>AMD-K6(tm)-III Processor</b></tt> (?)</td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">AMD K7</font></td>
  <td align=left>
   <tt><b>AMD-K7(tm) Processor</b></tt> <font face="Arial">(model 1)</font><br>
   <tt><b>AMD Athlon(tm) Processor</b></tt> <font face="Arial">(model 2)</font><br>
   <font face="Arial">newer models: programmable</font>
  </td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K8</font></td>
  <td align=left><font face="Arial">programmable via MSRs C001_0030h...C001_0035h, default is 48x 0</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD K8L</font></td>
  <td align=left><font face="Arial">programmable via MSRs C001_0030h...C001_0035h, default is 48x 0</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">AMD BC</font></td>
  <td align=left><font face="Arial">programmable via MSRs C001_0030h...C001_0035h, default is 48x 0</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">Geode GX2</font></td>
  <td align=left>
   <tt><b>Geode(TM) Integrated Processor by National Semi</b></tt><br>
   <font face="Arial">programmable via MSRs 0000_300Ah...0000_300Fh</font>
  </td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">Geode LX</font></td>
  <td align=left>
   <tt><b>Geode(TM) Integrated Processor by AMD PCS</b></tt><br>
   <font face="Arial">programmable via MSRs 0000_300Ah...0000_300Fh</font>
  </td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">Centaur C2 <sup>#2</sup></font></td>
  <td align=left>
   <tt><b>IDT WinChip 2</b></tt><br>
   <tt><b>IDT WinChip 2-3D</b></tt><br>
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Centaur C3</font></td>
  <td align=left><tt><b>IDT WinChip 3</b></tt></font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">VIA C3</font></td>
  <td align=left>
   <tt><b>CYRIX III(tm)</b></tt> (?)<br>
   <tt><b>VIA Samuel</b></tt> (?)<br>
   <tt><b>VIA Ezra</b></tt> (?)<br>
   <tt><b>VIA C3 Nehemiah</b></tt> (?)<br>
  </td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Intel PM <sup>#3</sup></font></td>
  <td align=left><tt><b>Intel(R) Pentium(R) M processor xxxxMHz</b></tt></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Intel P4 <sup>#3</sup></font></td>
  <td align=left><tt><b>Intel(R) Pentium(R) 4 CPU xxxxMHz</b></tt></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Intel Core 2</font></td>
  <td align=left><tt><b>Intel(R) Xeon(R) CPU &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; xxxx &nbsp;@ x.xxGHz</b></tt></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Crusoe</font></td>
  <td align=left><tt><b>Transmeta(tm) Crusoe(tm) Processor TMxxxx</b></tt></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Efficeon</font></td>
  <td align=left><tt><b>Transmeta Efficeon(tm) Processor TM8000</b></tt></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">Unused characters at the end of the string are filled with 00h.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">The string depends on whether 3DNow! is disabled or enabled.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#3</font></td>
  <td align=left colspan=3><font face="Arial">The string is right-justified, with leading whitespaces.</font></td>
 </tr>
</table>
<br>

<a name="level_8000_0005h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0005h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0005h</font></td>
  <td align=left colspan=2><font face="Arial">get L1 cache and L1 TLB configuration descriptors <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=24 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=6><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">4/2 MB L1 TLB configuration descriptor</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">data TLB associativity (FFh=full)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">data TLB entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">code TLB associativity (FFh=full)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">code TLB entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">4 KB L1 TLB configuration descriptor <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">data TLB associativity (FFh=full)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">data TLB entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">code TLB associativity (FFh=full)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">code TLB entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">ECX</font></td>
  <td align=left colspan=2><font face="Arial">data L1 cache configuration descriptor</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">data L1 cache size in KBs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">data L1 cache associativity (FFh=full)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">data L1 cache lines per tag</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">data L1 cache line size in bytes</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">EDX</font></td>
  <td align=left colspan=2><font face="Arial">code L1 cache configuration descriptor</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">code L1 cache size in KBs</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">code L1 cache associativity (FFh=full)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">code L1 cache lines per tag</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">code L1 cache line size in bytes</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">Cyrix processors return CPUID level 0000_0002h-like descriptors instead. (Though the NS Geode GX2 does not.)</font></td>
 </tr>
 <tr>
  <td valign=top align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">
   While Transmeta Crusoe processors have 256 entries, the CPUID definition constrains them to reporting only 255.<br>
   For compatibility reasons they report their unified TLB twice: once for the code TLB, and once for the data TLB.
  </font></td>
 </tr>
</table>
<br>

<a name="level_8000_0006h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0006h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0006h</font></td>
  <td align=left colspan=2><font face="Arial">get L2/L3 cache and L2 TLB configuration descriptors</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=25 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=6><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">4/2 MB L2 TLB configuration descriptor <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...28</font></td>
  <td align=left><font face="Arial">data TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27...16</font></td>
  <td align=left><font face="Arial">data TLB entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">code TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">code TLB entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">4 KB L2 TLB configuration descriptor <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...28</font></td>
  <td align=left><font face="Arial">data TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27...16</font></td>
  <td align=left><font face="Arial">data TLB entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">code TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">code TLB entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">ECX</font></td>
  <td align=left colspan=2><font face="Arial">unified L2 cache configuration descriptor <sup>#3</sup></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16 <sup>#5</sup></font></td>
  <td align=left><font face="Arial">unified L2 cache size in KBs <sup>#4</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12 <sup>#5</sup></font></td>
  <td align=left><font face="Arial">unified L2 cache associativity <sup>#2, #6</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...8 <sup>#5</sup></font></td>
  <td align=left><font face="Arial">unified L2 cache lines per tag</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">unified L2 cache line size in bytes</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=7><font face="Arial">EDX</font></td>
  <td align=left colspan=2><font face="Arial">unified L3 cache configuration descriptor</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...18</font></td>
  <td align=left><font face="Arial">unified L3 cache size in 512 KB chunks</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">unified L3 cache associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...8</font></td>
  <td align=left><font face="Arial">unified L3 cache lines per tag</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">unified L3 cache line size in bytes</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">A unified L2 TLB is indicated by a value of 0000h in the upper 16 bits.</font></td>
 </tr>
 <tr>
  <td valign=top align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">
   0000b=disabled,<br>
   0001b=1-way, 0010b=2-way, 0011b=3-way, 0100b=4-way, 0101b=6-way, 0110b=8-way, 1000b=16-way,<br>
   1001b=see level 8000_001Dh instead,<br>
   1010b=32-way, 1011b=48-way, 1100b=64-way, 1101b=96-way, 1110b=128-way, 1111b=full
  </font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#3</font></td>
  <td align=left colspan=3><font face="Arial">The AMD K7 processor's L2 cache must be configured prior to relying upon this information, if the model is 1 or 2.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#4</font></td>
  <td align=left colspan=3><font face="Arial">AMD K7 processors with CPUID=0630h (Duron) inadvertently report 1 KB instead of 64 KB.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#5</font></td>
  <td align=left colspan=3><font face="Arial">VIA C3 processors with CPUID=0670...068Fh (C5B/C5C) inadvertently use bits 31...24, 23...16, and 15...8 instead.</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#6</font></td>
  <td align=left colspan=3><font face="Arial">VIA C3 processors with CPUID=069x (C5XL) and stepping 1 inadvertently report 0 ways instead of 16 ways.</font></td>
 </tr>
</table>
<br>

<a name="level_8000_0007h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0007h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0007h</font></td>
  <td align=left colspan=2><font face="Arial">get capabilities</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=34 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=5><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">processor feedback capabilities</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">maximum wrap time in ms</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">version (01h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">number of monitors (MSR C001_008[01]h etc.)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=8><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">RAS capabilities</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (SCMCA)</font></td>
  <td align=left><font face="Arial">scalable MCA (more banks, MCA ext regs, DOER/SEER roles)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (HWA)</font></td>
  <td align=left><font face="Arial">hardware assert (MSR C001_10[DF...C0]h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (SUCCOR)</font></td>
  <td align=left><font face="Arial">software uncorrectable error containment and recovery</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (MCAOVR)</font></td>
  <td align=left><font face="Arial">MCA overflow recovery</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">ECX</font></td>
  <td align=left colspan=2><font face="Arial">advanced power monitoring interface</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0<br><font size="-2">(CmpUnitPwrSampleTimeRatio)</font></font></td>
  <td align=left><font face="Arial">ratio of power accumulator sample period to GTSC counter period</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=18><font face="Arial">EDX</font></td>
  <td align=left colspan=2><font face="Arial">enhanced power management capabilities</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...15</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14 (RAPL)</font></td>
  <td align=left><font face="Arial">running average power limit</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13 (CSB)</font></td>
  <td align=left><font face="Arial">connected standby</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12 (PA)</font></td>
  <td align=left><font face="Arial">processor accumulator (MSR C001_007Ah)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11 (PFI)</font></td>
  <td align=left><font face="Arial">processor feedback interface (see EAX)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10 (EFRO)</font></td>
  <td align=left><font face="Arial">read-only MPERF/APERF (MSR C000_00E[78]h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9 (CPB)</font></td>
  <td align=left><font face="Arial">core performance boost</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8 (ITSC)</font></td>
  <td align=left><font face="Arial">invariant TSC</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7 (HWPS)</font></td>
  <td align=left><font face="Arial">hardware P-state support</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6 (MUL100)</font></td>
  <td align=left><font face="Arial">100 MHz multiplier steps</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5 (STC)</font></td>
  <td align=left><font face="Arial">software thermal control</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4 (TM)</font></td>
  <td align=left><font face="Arial">thermal monitoring</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (TTP)</font></td>
  <td align=left><font face="Arial">thermal trip</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (VID)</font></td>
  <td align=left><font face="Arial">voltage ID control</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (FID)</font></td>
  <td align=left><font face="Arial">frequency ID control</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (TS)</font></td>
  <td align=left><font face="Arial">temperature sensor</font></td>
 </tr>
</table>
<br>

<a name="level_8000_0008h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0008h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0008h</font></td>
  <td align=left colspan=2><font face="Arial">get miscellaneous information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=36 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=6><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">address size information</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">guest physical address bits (if 0, then see bits 7...0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">virtual address bits</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">physical address bits</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=19><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">feature flags</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...19</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">18 (IBRS_PREF)</font></td>
  <td align=left><font face="Arial">IBRS preferred</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17 (STIBP_ALL)</font></td>
  <td align=left><font face="Arial">STIBP always on mode</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16 (IBRS_ALL)</font></td>
  <td align=left><font face="Arial">IBRS always on mode</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15 (STIBP)</font></td>
  <td align=left><font face="Arial">SPEC_CTRL.STIBP</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14 (IBRS)</font></td>
  <td align=left><font face="Arial">SPEC_CTRL.IBRS</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12 (IBPB)</font></td>
  <td align=left><font face="Arial">PRED_CMD.IBPB</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9 (WBNOINVD)</font></td>
  <td align=left><font face="Arial"><a href="opc_2.htm">WBNOINVD</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8 (MCOMMIT)</font></td>
  <td align=left><font face="Arial"><a href="msr.htm">EFER.MCOMMIT</a>, <a href="opc_grp.htm">MCOMMIT</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4 (RDPRU)</font></td>
  <td align=left><font face="Arial"><a href="crx.htm">CR4.TSD</a>, <a href="opc_grp.htm">RDPRU</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (ASRFPEP)</font></td>
  <td align=left><font face="Arial">always save/restore FP error pointers</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (IRPERF)</font></td>
  <td align=left><font face="Arial">read-only IRPERF (MSR C000_00E9h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (CLZERO)</font></td>
  <td align=left><font face="Arial"><a href="opc_grp.htm">CLZERO</a></font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=7><font face="Arial">ECX</font></td>
  <td align=left colspan=2><font face="Arial">processor count information</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...18</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">17...16</font></td>
  <td align=left><font face="Arial">performance TSC size (00b=40-bit, 01b=48-bit, 10b=56-bit, 11b=64-bit)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">number of LSBs in APIC ID that indicate core ID</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...8</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">cores per die - 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EDX</font></td>
  <td align=left colspan=2><font face="Arial">miscellaneous information</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">maximum valid ECX value for RDPRU (0=MPERF, 1=APERF)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...0</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
</table>
<br>

<a name="level_8000_000Ah">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_000Ah</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_000Ah</font></td>
  <td align=left colspan=2><font face="Arial">get SVM information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=28 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=5><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">revision and presence information</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...9</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">hypervisor present (and intercepting this bit, to advertise its presence)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">revision, starting at 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">address space information</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">number of ASIDs</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=20><font face="Arial">EDX</font></td>
  <td align=left colspan=2><font face="Arial">sub-feature information</font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...17</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">16 (VGIF)</font></td>
  <td align=left><font face="Arial">virtualized GIF</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15 (VLS)</font></td>
  <td align=left><font face="Arial">virtualized VMLOAD/VMSAVE</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">14</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13 (AVIC)</font></td>
  <td align=left><font face="Arial">AVIC</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">12 (<font size="-2">PAUSEFILTERTHR.</font>)</font></td>
  <td align=left><font face="Arial">PAUSE filter threshold</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10 (<font size="-1">PAUSEFILTER</font>)</font></td>
  <td align=left><font face="Arial">PAUSE intercept filter</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9 (<font size="-1">SSSE3SSE5ADIS</font>)</font></td>
  <td align=left><font face="Arial">SSSE3 and SSE5A disable</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7 (<font size="-1">DECODEASSISTS</a>)</font></td>
  <td align=left><font face="Arial">decode assists</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6 (<font size="-1">FLUSHBYASID</font>)</font></td>
  <td align=left><font face="Arial">flush by ASID</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5 (<font size="-1">VMCBCLEAN</font>)</font></td>
  <td align=left><font face="Arial">VMCB clean bits</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4 (<font size="-1">TSCRATEMSR</a>)</font></td>
  <td align=left><font face="Arial">MSR-based TSC rate control</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3 (NRIPS)</font></td>
  <td align=left><font face="Arial">NRIP save on #VMEXIT</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (SVML)</font></td>
  <td align=left><font face="Arial">SVM lock</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (LBRV)</font></td>
  <td align=left><font face="Arial">LBR virtualization</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (NP)</font></td>
  <td align=left><font face="Arial">nested paging</font></td>
 </tr>
</table>
<br>

<a name="level_8000_0019h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_0019h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_0019h</font></td>
  <td align=left colspan=2><font face="Arial">get TLB configuration descriptors</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=6><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">1 GB L1 TLB configuration descriptor <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...28</font></td>
  <td align=left><font face="Arial">data TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27...16</font></td>
  <td align=left><font face="Arial">data TLB entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">code TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">code TLB entries</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">1 GB L2 TLB configuration descriptor <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...28</font></td>
  <td align=left><font face="Arial">data TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27...16</font></td>
  <td align=left><font face="Arial">data TLB entries</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...12</font></td>
  <td align=left><font face="Arial">code TLB associativity <sup>#2</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">code TLB entries</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">notes</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">descriptions</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">A unified TLB is indicated by a value of 0000h in the upper 16 bits.</font></td>
 </tr>
 <tr>
  <td valign=top align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#2</font></td>
  <td align=left colspan=3><font face="Arial">
   0000b=disabled, 0001b=1-way, 0010b=2-way, 0100b=4-way, 0110b=8-way, 1000b=16-way,<br>
   1010b=32-way, 1011b=48-way, 1100b=64-way, 1101b=96-way, 1110b=128-way, 1111b=full
  </font></td>
 </tr>
</table>
<br>

<a name="level_8000_001Ah">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_001Ah</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_001Ah</font></td>
  <td align=left colspan=2><font face="Arial">get performance optimization identifiers</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=6><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">performance optimization identifiers</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...3</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2 (FP256)</font></td>
  <td align=left><font face="Arial">1x 256-bit instead of 2x 128-bit processing</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1 (MOVU)</font></td>
  <td align=left><font face="Arial">prefer unaligned MOV over MOVL/MOVH</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0 (FP128)</font></td>
  <td align=left><font face="Arial">1x 128-bit instead of 2x 64-bit processing</font></td>
 </tr>
</table>
<br>

<a name="level_8000_001Bh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_001Bh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_001Bh</font></td>
  <td align=left colspan=2><font face="Arial">get IBS information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=14 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=14><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">IBS feature flags</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...11</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10</font></td>
  <td align=left><font face="Arial">IBS op data 4 MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">IBS fetch control extended MSR</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">fused branch micro-op indication</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7</font></td>
  <td align=left><font face="Arial">invalid RIP indication</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">IbsOpCurCnt and IbsOpMaxCnt extend by 7 bits</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">branch target address reporting</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">op counting mode</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">read write of op counter</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">IBS execution sampling</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">IBS fetch sampling</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">IBS feature flags valid</font></td>
 </tr>
</table>
<br>

<a name="level_8000_001Ch">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_001Ch</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8000_001Ch</font></td>
  <td align=left colspan=2><font face="Arial">get LWP information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=40 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=12 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31</font></td>
  <td align=left><font face="Arial">interrupt on threshold overflow available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30</font></td>
  <td align=left><font face="Arial">performance time stamp counter in event record available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29</font></td>
  <td align=left><font face="Arial">sampling in continuous mode available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">28...7</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">core reference clocks not halted event available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">core clocks not halted event available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">DC miss event available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">branch retired event available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">instructions retired event available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">LWPVAL instruction available</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">LWP available (copy of <a href="crx.htm">XCR0.LWP</a>)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...24</font></td>
  <td align=left><font face="Arial">EventInterval1 field offset</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">maximum EventId</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">event record size</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">control block size</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=11><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31</font></td>
  <td align=left><font face="Arial">cache latency filtering supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30</font></td>
  <td align=left><font face="Arial">cache level filtering supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29</font></td>
  <td align=left><font face="Arial">IP filtering supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">28</font></td>
  <td align=left><font face="Arial">branch prediction filtering supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">27...24</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">23...16</font></td>
  <td align=left><font face="Arial">event ring buffer size</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">15...9</font></td>
  <td align=left><font face="Arial">version</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8...6</font></td>
  <td align=left><font face="Arial">amount by which cache latency is rounded</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">data cache miss address valid</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">latency counter bit size</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31</font></td>
  <td align=left><font face="Arial">interrupt on threshold overflow supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">30</font></td>
  <td align=left><font face="Arial">performance time stamp counter in event record supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">29</font></td>
  <td align=left><font face="Arial">sampling in continuous mode supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">28...7</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">6</font></td>
  <td align=left><font face="Arial">core reference clocks not halted event supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">core clocks not halted event supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">DC miss event supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">branch retired event supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">instructions retired event supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">LWPVAL instruction supported</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">LWP supported (copy of LWP feature flag in extended level 8000_0001h)</font></td>
 </tr>
</table>
<br>

<a name="level_8000_001Dh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_001Dh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=2 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8000_001Dh</font></td>
  <td align=left colspan=2><font face="Arial">get cache configuration descriptors</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">cache level to query (until EAX reports cache type = 0)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=18 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=8 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...26</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">25...14</font></td>
  <td align=left><font face="Arial">cores per cache - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">13...10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">9</font></td>
  <td align=left><font face="Arial">fully associative?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">8</font></td>
  <td align=left><font face="Arial">self-initializing?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...5</font></td>
  <td align=left><font face="Arial">cache level (starts at 1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4...0</font></td>
  <td align=left><font face="Arial">cache type (0=null, 1=data, 2=code, 3=unified, 4...31=reserved)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...22</font></td>
  <td align=left><font face="Arial">ways of associativity - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">21...12</font></td>
  <td align=left><font face="Arial">physical line partitions - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...0</font></td>
  <td align=left><font face="Arial">system coherency line size - 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">sets - 1</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...2</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">inclusive of lower levels?</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">write-back invalidate?</font></td>
 </tr>
</table>
<br>

<a name="level_8000_001Eh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_001Eh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=8000_001Eh</font></td>
  <td align=left colspan=2><font face="Arial">get APIC/unit/node information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=13 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=3><font face="Arial">EAX</font></td>
  <td align=left colspan=2><font face="Arial">extended APIC ID</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">extended APIC ID</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">EBX</font></td>
  <td align=left colspan=2><font face="Arial">
   compute unit identifiers (Fam 15h)<br>
   core identifiers (Fam 17h)
  </font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...16</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">15...8</font></td>
  <td align=left><font face="Arial">
   cores per compute unit - 1 (Fam 15h)<br>
   threads per core - 1 (Fam 17h)
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">
   compute unit ID (Fam 15h)<br>
   core ID (Fam 17h)
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5><font face="Arial">ECX</font></td>
  <td align=left colspan=2><font face="Arial">node identifiers</font></td>
 </tr>
 <tr>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...11</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">10...8</font></td>
  <td align=left><font face="Arial">nodes per processor - 1</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">7...0</font></td>
  <td align=left><font face="Arial">node ID</font></td>
 </tr>
</table>
<br>

<a name="level_8000_001Fh">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">extended level 8000_001Fh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8000_001Fh</font></td>
  <td align=left colspan=2><font face="Arial">get SME/SEV information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=15 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=7 width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...5</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">SEV-ES, VMGEXIT, #VC, GHCB MSR (C001_0130h)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">VMPAGE_FLUSH MSR (C001_011Eh)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">1</font></td>
  <td align=left><font face="Arial">SEV</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">SME</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4><font face="Arial">EBX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...12</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">11...6</font></td>
  <td align=left><font face="Arial">hPA bit count reduction when memory encryption is active</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">5...0</font></td>
  <td align=left><font face="Arial">page table bit position used to indicate memory encryption</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">ECX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">number of simultaneously supported encrypted guests</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EDX</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">bits</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">31...0</font></td>
  <td align=left><font face="Arial">minimum SEV enabled, SEV-ES disabled ASID</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<a name="level_8086_0000h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Transmeta level 8086_0000h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8086_0000h</font></td>
  <td align=left colspan=2><font face="Arial">get maximum supported level and vendor ID string</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center width="18%"><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left colspan=2><font face="Arial">maximum supported level</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2><font face="Arial">EBX-EDX-ECX</font></td>
  <td align=left colspan=2><font face="Arial">vendor ID string</font></td>
 </tr>
 <tr>
  <td align=center width="18%"><tt><b>TransmetaCPU</b></tt></td>
  <td width="58%" align=left><font face="Arial">Transmeta processor</font></td>
 </tr>
</table>
<br>

<a name="level_8086_0001h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=6 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Transmeta level 8086_0001h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8086_0001h</font></td>
  <td align=left colspan=4><font face="Arial">get processor information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td width=50 valign=top align=center rowspan=25 bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td width=160 valign=top align=center rowspan=16><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left colspan=4><font face="Arial">processor family/model/stepping</font></td>
 </tr>
 <tr>
  <td width=160 valign=top align=center rowspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">
   extended family<br>
   (add)
  </font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The extended processor family is encoded in bits 27...20.</font></td>
 </tr>
 <tr>
  <td width=160 align=center></td>
  <td width=50 valign=top align=center><font face="Arial">00<font color="#808080">+0</font></font></td>
  <td width=296 align=left><font face="Arial">Transmeta Efficeon</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">
   extended model<br>
   (concat)
  </font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The extended processor model is encoded in bits 19...16.</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Crusoe</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">TM3x00 and TM5x00</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Efficeon</font></td>
  <td align=center><font face="Arial">0</font></td>
  <td align=left><font face="Arial">TM8000</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">family</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The family is encoded in bits 11...8.</font></td>
 </tr>
 <tr>
  <td align=center rowspan=2></td>
  <td align=center><font face="Arial">5</font></td>
  <td align=left><font face="Arial">Transmeta Crusoe</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">F</font></td>
  <td align=left><font face="Arial">refer to extended family</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=5 bgcolor="#004080"><font color="#FFFFFF" face="Arial">model</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The model is encoded in bits 7...4.</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">Transmeta Crusoe</font></td>
  <td align=center><font face="Arial">4</font></td>
  <td align=left><font face="Arial">TM3x00 and TM5x00</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=3><font face="Arial">Transmeta Efficeon</font></td>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">TM8000 (130 nm)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">2</font></td>
  <td align=left><font face="Arial">TM8000 (90 nm CMS 6.0)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">3</font></td>
  <td align=left><font face="Arial">TM8000 (90 nm CMS 6.1+)</font></td>
 </tr>
 <tr>
  <td align=center valign=top rowspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">stepping</font></td>
  <td align=left colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">The stepping is encoded in bits 3...0.</font></td>
 </tr>
 <tr>
  <td align=left colspan=3><font face="Arial">The stepping values are processor-specific.</font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">EBX=aabb_ccddh</font></td>
  <td valign=top align=left colspan=4><font face="Arial">
   hardware revision (a.b-c.d)<br>
   0101_xxyyh = TM3200<br>
   0102_xxyyh = TM5400<br>
   0103_xxyyh = TM5400 or TM5600<br>
   0103_00yyh = TM5500 or TM5800<br>
   0104_xxyyh = TM5500 or TM5800<br>
   0105_xxyyh = TM5500 or TM5800<br>
   0200_0000h = see level 8086_0002h register EAX
  </font></td>
 </tr>
 <tr>
  <td valign=top align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left colspan=4><font face="Arial">nominal core clock frequency (MHz)</font></td>
 </tr>
 <tr>
  <td valign=top align=center rowspan=6><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of indicated feature</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 31...4</font></td>
  <td align=left colspan=3><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 3 (LRTI)</font></td>
  <td align=left colspan=3><font face="Arial">LongRun Table Interface</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 2 (???)</font></td>
  <td align=left colspan=3><font face="Arial">unknown</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 1 (LR)</font></td>
  <td align=left colspan=3><font face="Arial">LongRun</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 0 (BAD)</font></td>
  <td align=left colspan=3><font face="Arial">recovery CMS active (due to a failed upgrade)</font></td>
 </tr>
</table>
<br>

<a name="level_8086_0002h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Transmeta level 8086_0002h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center ><font face="Arial">EAX=8086_0002h</font></td>
  <td align=left colspan=2><font face="Arial">get processor information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=3 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center width="18%"><font face="Arial">EAX</font></td>
  <td valign=top align=center width="18%"><font face="Arial">xxxx_xxxxh</font></td>
  <td width="58%" align=left><font face="Arial">
   reserved or hardware revision (xxxxxxxxh)<br>
   see level 8086_0001h register EBX
  </font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EBX</font></td>
  <td align=center><font face="Arial">aabb_ccddh</font></td>
  <td align=left><font face="Arial">software revision, part 1/2 (a.b.c-d-x)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX</font></td>
  <td align=center><font face="Arial">xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">software revision, part 2/2 (a.b.c-d-x)</font></td>
 </tr>
</table>
<br>

<a name="level_8086_0003h">
<a name="level_8086_0004h">
<a name="level_8086_0005h">
<a name="level_8086_0006h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Transmeta levels 8086_0003h, 8086_0004h, 8086_0005h, and 8086_0006h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=4 align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8086_0003h</font></td>
  <td align=left colspan=2><font face="Arial">get information string (part 1)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EAX=8086_0004h</font></td>
  <td align=left colspan=2><font face="Arial">get information string (part 2)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EAX=8086_0005h</font></td>
  <td align=left colspan=2><font face="Arial">get information string (part 3)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EAX=8086_0006h</font></td>
  <td align=left colspan=2><font face="Arial">get information string (part 4)</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=2 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=2 width="18%"><font face="Arial">
   EAX-EBX-ECX-EDX
  </font></td>
  <td align=left colspan=2><font face="Arial">information string <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td width="18%" align=center><font face="Arial">Transmeta</font></td>
  <td width="58%" align=left><tt><b>20000805 23:30 official release 4.1.4#2</b></tt> (example)</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">Unused characters at the end of the string are filled with 00h.</font></td>
 </tr>
</table>
<br>

<a name="level_8086_0007h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Transmeta level 8086_0007h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8086_0007h</font></td>
  <td align=left colspan=2><font face="Arial">get processor information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%"><font face="Arial">xxxx_xxxxh</font></td>
  <td width="58%" align=left><font face="Arial">current core clock frequency (MHz)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EBX</font></td>
  <td align=center><font face="Arial">xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">current core clock voltage (mV)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX</font></td>
  <td align=center><font face="Arial">xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">current (LongRun) performance level (0...100%)</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EDX</font></td>
  <td align=center><font face="Arial">xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">current gate delay (fs)</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<a name="level_C000_0000h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=3 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Centaur level C000_0000h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=C000_0000h</font></td>
  <td align=left width="76%"><font face="Arial">get maximum supported level</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">maximum supported level</font></td>
 </tr>
</table>
<br>

<a name="level_C000_0001h">
<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">Centaur level C000_0001h</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=C000_0001h</font></td>
  <td align=left colspan=2><font face="Arial">get processor information</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=12 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td valign=top align=center rowspan=12 width="18%"><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=center width="18%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">feature flags</font></td>
  <td align=center width="58%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">description of indicated feature</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bits 31...10</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 9 (MM/HE-E)</font></td>
  <td align=left><font face="Arial">Montgomery Multiplier and Hash Engine enabled</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 8 (MM/HE)</font></td>
  <td align=left><font face="Arial">Montgomery Multiplier and Hash Engine</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 7 (ACE-E)</font></td>
  <td align=left><font face="Arial">Advanced Cryptography Engine enabled</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 6 (ACE)</font></td>
  <td align=left><font face="Arial">Advanced Cryptography Engine</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 5 (FEMMS)</font></td>
  <td align=left><font face="Arial"><a href="opc_k3d.htm">FEMMS</a></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 4 (LH)</font></td>
  <td align=left><font face="Arial">LongHaul MSR 0000_110Ah</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 3 (RNG-E)</font></td>
  <td align=left><font face="Arial">Random Number Generator enabled</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 2 (RNG)</font></td>
  <td align=left><font face="Arial">Random Number Generator</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 1 (AIS-E)</font></td>
  <td align=left><font face="Arial">Alternate Instruction Set enabled</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">bit 0 (AIS)</font></td>
  <td align=left><font face="Arial">Alternate Instruction Set</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">mystery level 8FFF_FFFEh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8FFF_FFFEh</font></td>
  <td align=left colspan=2><font face="Arial">unknown <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center rowspan=4 bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center width="18%"><font face="Arial">EAX</font></td>
  <td align=center width="18%"><font face="Arial">0049_4544h</font></td>
  <td width="58%" align=left><tt><b><a href="http://www.ucolick.org/~sla/dei/" target="_blank">DEI</a></b></tt></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EBX</font></td>
  <td align=center><font face="Arial">0000_0000h</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX</font></td>
  <td align=center><font face="Arial">0000_0000h</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EDX</font></td>
  <td align=center><font face="Arial">0000_0000h</font></td>
  <td align=left><font face="Arial">reserved</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only supported by the AMD K6 processor family.</font></td>
 </tr>
</table>
<br>

<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=4 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">mystery level 8FFF_FFFFh</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center><font face="Arial">EAX=8FFF_FFFFh</font></td>
  <td align=left colspan=2><font face="Arial">unknown <sup>#1</sup></font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center bgcolor="#004080" width="6%"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center width="18%"><font face="Arial">
   EAX<br>
   EBX<br>
   ECX<br>
   EDX<br>
  </font></td>
  <td valign=top align=center width="18%"><font face="Arial">string</font></td>
  <td valign=top width="58%" align=left>
   <tt><b>NexGenerationAMD</b></tt> (K6)<br>
   <tt><b>IT'S HAMMER TIME</b></tt> (K8)<br>
   <tt><b>HELLO KITTY! ^-^</b></tt> (KB)
  </td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=3 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=3><font face="Arial">This level is only supported by the indicated processor families.</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=3 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">AMD SimNow! level BACC_D00Ah</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top rowspan=5 align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=BACC_D00Ah</font></td>
  <td align=left width="76%"><font face="Arial">backdoor call <sup>#1</sup></font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EDI=CA11_xxxxh</font></td>
  <td align=left><font face="Arial">function number</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EBX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">1st argument</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">ECX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">2nd argument</font></td>
 </tr>
 <tr>
  <td align=center><font face="Arial">EDX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">3rd argument</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left><font face="Arial">return value</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">note</font></td>
  <td align=center colspan=2 bgcolor="#004080"><font color="#FFFFFF" face="Arial">description</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">#1</font></td>
  <td align=left colspan=2><font face="Arial">This level is only supported by the AMD SimNow! simulator.</font></td>
 </tr>
</table>
<br>

<hr>
<br>

<table width=900 border=1 cellspacing=0 cellpadding=2>
 <tr>
  <td align=center colspan=3 bgcolor="#004080">&nbsp;<br><font size=+2 color="#FFFFFF" face="Arial">all other levels</font><br>&nbsp;</td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td align=center width="6%" bgcolor="#004080"><font color="#FFFFFF" face="Arial">input</font></td>
  <td align=center width="18%"><font face="Arial">EAX=xxxx_xxxxh</font></td>
  <td align=left width="76%"><font face="Arial">desired CPUID level</font></td>
 </tr>
 <tr>
 </tr>
 <tr>
  <td valign=top align=center bgcolor="#004080"><font color="#FFFFFF" face="Arial">output</font></td>
  <td align=center><font face="Arial">
   EAX=xxxx_xxxxh<br>
   EBX=xxxx_xxxxh<br>
   ECX=xxxx_xxxxh<br>
   EDX=xxxx_xxxxh<br>
  </font></td>
  <td valign=top align=left><font face="Arial">undefined</font></td>
 </tr>
</table>
