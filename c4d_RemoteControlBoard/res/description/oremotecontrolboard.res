CONTAINER Oremotecontrolboard
{
	NAME Oremotecontrolboard;
	INCLUDE Obase;

	GROUP ID_OBJECTPROPERTIES
	{
		STRING BODY_PART { }
		GROUP ID_GROUP_CONTROL
		{
			LAYOUTGROUP; COLUMNS 3; DEFAULT 1;

			GROUP
			{
				BUTTON CONNECT_BUTTON { NAME CONNECT_TEXT; }
			}

			GROUP
			{
				BUTTON DISCONNECT_BUTTON { NAME DISCONNECT_TEXT; }
			}

			GROUP
			{
				BUTTON CONFIGURE_BUTTON { NAME CONFIGURE_TEXT; }
			}
			
			
		}
		SEPARATOR
		{
			LINE;
		}
		
		GROUP ID_GROUP_CONTROL_MODE
		{
			LAYOUTGROUP; COLUMNS 2; DEFAULT 1;
			GROUP
			{
				BUTTON SENDPOS_BUTTON { NAME SENDPOS_TEXT; }
			}
			GROUP
			{
				LONG CONTROL_MODE
				{
					CYCLE
					{
						CONTROL_MODE_POSITION;
						CONTROL_MODE_POSITION_DIRECT;
					}
				}
			}
		}
		
		SEPARATOR
		{
			LINE;
		}
		LONG JOINT_COUNT	{ MIN 0; MAX 50; }
	}
}
