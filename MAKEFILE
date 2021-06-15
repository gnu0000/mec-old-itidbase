#+--------------------------------------------------------------------------+
#|                                                                          |
#|       Copyright (c) 1992 by Info Tech, Inc.  All Rights Reserved.        |
#|                                                                          |
#|       This program module is part of DS/Shell (PC), Info Tech's          |
#|       database interfaces for OS/2, a proprietary product of             |
#|       Info Tech, Inc., no part of which may be reproduced or             |
#|       transmitted in any form or by any means, electronic,               |
#|       mechanical, or otherwise, including photocopying and recording     |
#|       or in connection with any information storage or retrieval         |
#|       system, without permission in writing from Info Tech, Inc.         |
#|                                                                          |
#+--------------------------------------------------------------------------+



# make file for data base dll
!include "..\makeopts"

TARGET=itidbase

all: $(EXEDIR)$(TARGET).dll $(LIBDIR)$(TARGET).lib

$(EXEDIR)$(TARGET).dll: *.c *.h ..\include\*.h $(TARGET).def *.dlg
   premake /AddDot $(TARGET).pre $(TARGET)
   nmake -nologo -f $(TARGET)

$(LIBDIR)$(TARGET).lib: $(TARGET).def
   premake /AddDot $(TARGET).pre $(TARGET)
   nmake -nologo -f $(TARGET) $*.lib


