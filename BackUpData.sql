USE [Study]
GO
Delete From T_State

INSERT INTO [dbo].[T_State]
           ([K_StateCode]
           ,[F_StateName]
           ,[F_Pinyin]
           ,[F_SCode]
           ,[F_AllName]
           ,[F_Tel])
     Select * from T_State_Back


