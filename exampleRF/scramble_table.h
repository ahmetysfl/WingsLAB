const uint8_t const scramble_table[40][42] = {
{64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, },
{137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, },
{210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, },
{27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, },
{100, 121, 135, 63, 110, 148, 190, 10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, },
{173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, },
{246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, },
{63, 110, 148, 190, 10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, },
{8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, },
{193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, },
{154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, },
{83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, },
{44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, },
{229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, 110, 148, 190, 10, 237, 57, 53, },
{190, 10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, },
{119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, },
{208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, },
{25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, 110, },
{66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, },
{139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, },
{244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, },
{61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, },
{102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, },
{175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, },
{152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, },
{81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, 110, 148, 190, 10, 237, 57, 53, 131, 173, 139, 137, },
{10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, },
{195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, },
{188, 195, 31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, },
{117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, },
{46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, 110, 148, 190, 10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, },
{231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, 110, 148, 190, 10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, },
{96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, },
{169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, },
{242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, 91, 23, 19, 129, 100, 121, 135, 63, 110, 148, 190, 10, 237, 57, 53, 131, 173, 139, 137, 64, 178, 188, 195, 31, 55, 74, 95, 133, 246, 156, 154, },
{59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, 4, 146, 229, 29, 254, 184, 81, 250, 42, 180, 231, 212, 12, 182, 46, 38, 2, 201, 242, 14, 127, 220, 40, 125, 21, 218, 115, 106, 6, },
{68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, },
{141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, 171, 208, 158, 83, 51, 216, 186, 152, 8, 36, 203, 59, 252, 113, 163, 244, 85, 104, 207, 169, 25, 108, 93, 76, },
{214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, 49, 17, 72, 150, 119, 248, 227, 70, 233, },
{31, 55, 74, 95, 133, 246, 156, 154, 193, 214, 197, 68, 32, 89, 222, 225, 143, 27, 165, 175, 66, 123, 78, 205, 96, 235, 98, 34, 144, 44, 239, 240, 199, 141, 210, 87, 161, 61, 167, 102, 176, 117, },
};

